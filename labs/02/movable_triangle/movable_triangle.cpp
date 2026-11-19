/**
 * @file movable_triangle.cpp
 * @author FaunJyng
 * @brief Use ImGui to interactively reposition the triangle's three vertices.
 * @version 2.0
 * @date 16/11/2026
 */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

#include <cstddef>
#include <filesystem>
#include <memory>

namespace {
	struct Vertex {
		float position[2];
		float color[3];
	};

	using UniqueSDLWindow =
		std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

	using UniqueSDLGPUDevice =
		std::unique_ptr<SDL_GPUDevice, decltype(&SDL_DestroyGPUDevice)>;

	template <typename T, auto ReleaseFunction>
	struct GPUDResourceCleaner {
		SDL_GPUDevice* device{nullptr};
		void operator()(T* resource) const noexcept {
			if (resource) ReleaseFunction(device, resource);
		}
	};

	using UniqueGPUShader = std::unique_ptr<SDL_GPUShader,
		GPUDResourceCleaner<SDL_GPUShader, SDL_ReleaseGPUShader>>;

	using UniqueGPUBuffer = std::unique_ptr<SDL_GPUBuffer,
		GPUDResourceCleaner<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>>;

	using UniqueGPUTransferBuffer = std::unique_ptr<SDL_GPUTransferBuffer,
		GPUDResourceCleaner<SDL_GPUTransferBuffer,
			SDL_ReleaseGPUTransferBuffer>>;

	using UniqueGPUGraphicsPipeline = std::unique_ptr<SDL_GPUGraphicsPipeline,
		GPUDResourceCleaner<SDL_GPUGraphicsPipeline,
			SDL_ReleaseGPUGraphicsPipeline>>;

	/*
	 * Declared before GPU resources in AppState.
	 * Reverse member-destruction order therefore releases pipelines/buffers
	 * before releasing the window claim, and destroys the device afterwards.
	 */
	struct WindowClaimManger {
		SDL_GPUDevice* device{nullptr};
		SDL_Window* window{nullptr};

		WindowClaimManger() = default;

		WindowClaimManger(const WindowClaimManger&) = delete;
		auto operator=(const WindowClaimManger&) -> WindowClaimManger& = delete;
		WindowClaimManger(WindowClaimManger&&) = delete;
		auto operator=(WindowClaimManger&&) -> WindowClaimManger& = delete;

		~WindowClaimManger() {
			if (device && window)
				SDL_ReleaseWindowFromGPUDevice(device, window);
		}
	};

	struct ImGuiManager {
		bool imgui_initialized{false};

		auto Init(SDL_Window* window, SDL_GPUDevice* device) -> bool {
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();

			ImGuiIO& io{ImGui::GetIO()};
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

			ImGui::StyleColorsLight();
			if (!ImGui_ImplSDL3_InitForSDLGPU(window)) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					"ImGui_ImplSDL3_InitForSDLGPU() failed");
				ImGui::DestroyContext();
				return false;
			}

			SDL_GPUTextureFormat format{
				SDL_GetGPUSwapchainTextureFormat(device, window)};
			if (format == SDL_GPU_TEXTUREFORMAT_INVALID) {
				ImGui_ImplSDL3_Shutdown();
				ImGui::DestroyContext();
				return false;
			}

			ImGui_ImplSDLGPU3_InitInfo init_info{};
			init_info.Device = device;
			init_info.ColorTargetFormat = format;
			init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
			if (!ImGui_ImplSDLGPU3_Init(&init_info)) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					"ImGui_ImplSDLGPU3_Init() failed");
				ImGui_ImplSDL3_Shutdown();
				ImGui::DestroyContext();
				return false;
			}

			imgui_initialized = true;
			return true;
		}

		~ImGuiManager() {
			if (imgui_initialized) {
				ImGui_ImplSDL3_Shutdown();
				ImGui_ImplSDLGPU3_Shutdown();
				ImGui::DestroyContext();
			}
		}
	};

	namespace fs = std::filesystem;
	template <typename... Args>
	auto GetAbsolutePathRelativeToExe(std::string* output, Args&&... paths)
		-> bool {
		if (!output) return false;

		const char* base_path{SDL_GetBasePath()};
		if (!base_path) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_GetBasePath() failed: %s",
				SDL_GetError());
			return false;
		}

		fs::path full_path{base_path};
		(full_path /= ... /= std::forward<Args>(paths));

		*output = full_path.string();
		return true;
	}

	auto LoadShader(SDL_GPUDevice* device, const char* path,
		SDL_GPUShaderStage stage) -> UniqueGPUShader {
		if (!device || !path) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"LoadShader(): invalid device or path");
			return {};
		}

		size_t code_size{0};

		std::unique_ptr<void, decltype(&SDL_free)> code{
			SDL_LoadFile(path, &code_size),
			SDL_free};

		if (!code) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_LoadFile() failed (%s): %s",
				path,
				SDL_GetError());
			return {};
		}

		if (code_size == 0) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"Shader file is empty: %s",
				path);
			return {};
		}

		SDL_GPUShaderCreateInfo info{};
		info.code_size = code_size;
		info.code = static_cast<const Uint8*>(code.get());
		info.entrypoint = "main";
		info.format = SDL_GPU_SHADERFORMAT_SPIRV;
		info.stage = stage;

		SDL_GPUShader* shader{SDL_CreateGPUShader(device, &info)};
		if (!shader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_CreateGPUShader() failed (%s): %s",
				path,
				SDL_GetError());
			return {};
		}

		return UniqueGPUShader{shader,
			GPUDResourceCleaner<SDL_GPUShader, SDL_ReleaseGPUShader>{device}};
	}

	auto CreateTriangleGraphicsPipeline(SDL_GPUDevice* device,
		SDL_Window* window) -> UniqueGPUGraphicsPipeline {
		std::string vertex_shader_path, fragment_shader_path;
		if (!GetAbsolutePathRelativeToExe(&vertex_shader_path,
				"02",
				"vertex_buffer_triangle",
				"shaders",
				"triangle.vert.spv") ||
			!GetAbsolutePathRelativeToExe(&fragment_shader_path,
				"02",
				"vertex_buffer_triangle",
				"shaders",
				"triangle.frag.spv")) {
			return nullptr;
		}

		UniqueGPUShader vertex_shader{LoadShader(device,
			vertex_shader_path.c_str(),
			SDL_GPU_SHADERSTAGE_VERTEX)};
		if (!vertex_shader) return nullptr;

		UniqueGPUShader fragment_shader{LoadShader(device,
			fragment_shader_path.c_str(),
			SDL_GPU_SHADERSTAGE_FRAGMENT)};
		if (!fragment_shader) return nullptr;

		SDL_GPUColorTargetDescription color_target{};
		color_target.format = SDL_GetGPUSwapchainTextureFormat(device, window);

		SDL_GPUVertexBufferDescription vertex_buf_desc{};
		vertex_buf_desc.slot = 0;
		vertex_buf_desc.pitch = sizeof(Vertex);
		vertex_buf_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
		vertex_buf_desc.instance_step_rate = 0;

		SDL_GPUVertexAttribute vertex_attrs[2]{};
		vertex_attrs[0].buffer_slot = 0;
		vertex_attrs[0].location = 0;
		vertex_attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		vertex_attrs[0].offset = offsetof(Vertex, position);

		vertex_attrs[1].buffer_slot = 0;
		vertex_attrs[1].location = 1;
		vertex_attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertex_attrs[1].offset = offsetof(Vertex, color);

		SDL_GPUGraphicsPipelineCreateInfo info{};

		info.vertex_shader = vertex_shader.get();
		info.fragment_shader = fragment_shader.get();

		info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

		info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;

		info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

		info.target_info.color_target_descriptions = &color_target;
		info.target_info.num_color_targets = 1;

		info.vertex_input_state.vertex_buffer_descriptions = &vertex_buf_desc;
		info.vertex_input_state.num_vertex_buffers = 1;
		info.vertex_input_state.vertex_attributes = vertex_attrs;
		info.vertex_input_state.num_vertex_attributes = 2;

		SDL_GPUGraphicsPipeline* pipeline{
			SDL_CreateGPUGraphicsPipeline(device, &info)};
		if (!pipeline) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_CreateGPUGraphicsPipeline() failed: %s",
				SDL_GetError());
		}

		return UniqueGPUGraphicsPipeline{pipeline,
			GPUDResourceCleaner<SDL_GPUGraphicsPipeline,
				SDL_ReleaseGPUGraphicsPipeline>{device}};
	}

	constexpr Vertex INITIAL_VERTICES[3]{
		{{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
		{{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
	};

	struct AppState {
		UniqueSDLWindow window{nullptr, SDL_DestroyWindow};
		UniqueSDLGPUDevice device{nullptr, SDL_DestroyGPUDevice};
		// Pipeline must be released before GPU device
		WindowClaimManger window_claim_manager;

		Vertex vertices[3]{INITIAL_VERTICES[0],
			INITIAL_VERTICES[1],
			INITIAL_VERTICES[2]};

		UniqueGPUGraphicsPipeline pipeline{nullptr};
		UniqueGPUBuffer vertex_buffer{nullptr};
		UniqueGPUTransferBuffer transfer_buffer{nullptr};

		ImGuiManager imgui_manager;

		~AppState() {
			if (device) SDL_WaitForGPUIdle(device.get());
		}
	};

	auto CreateVertexResources(AppState& appstate) -> bool {
		const Uint32 size{static_cast<Uint32>(sizeof(appstate.vertices))};

		SDL_GPUBufferCreateInfo buffer_info{};
		buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		buffer_info.size = size;
		appstate.vertex_buffer = UniqueGPUBuffer{
			SDL_CreateGPUBuffer(appstate.device.get(), &buffer_info),
			GPUDResourceCleaner<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>{
				appstate.device.get()}};
		if (!appstate.vertex_buffer) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_CreateGPUBuffer() failed: %s",
				SDL_GetError());
			return false;
		}

		SDL_GPUTransferBufferCreateInfo transfer_buffer_info{};
		transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transfer_buffer_info.size = size;
		appstate.transfer_buffer = UniqueGPUTransferBuffer{
			SDL_CreateGPUTransferBuffer(appstate.device.get(),
				&transfer_buffer_info),
			GPUDResourceCleaner<SDL_GPUTransferBuffer,
				SDL_ReleaseGPUTransferBuffer>{appstate.device.get()}};
		return true;
	}

	auto UploadVertices(AppState& appstate,
		SDL_GPUCommandBuffer* command_buffer) -> bool {
		SDL_GPUDevice* device{appstate.device.get()};
		const Uint32 size{static_cast<Uint32>(sizeof(appstate.vertices))};

		/** cycle = true: If GPU still reading last frame data, SDL allocate new
		 * memory */
		void* mapped{SDL_MapGPUTransferBuffer(device,
			appstate.transfer_buffer.get(),
			true)};
		if (!mapped) return false;
		SDL_memcpy(mapped, appstate.vertices, size);
		SDL_UnmapGPUTransferBuffer(device, appstate.transfer_buffer.get());

		SDL_GPUCopyPass* copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
		if (!copy_pass) return false;

		SDL_GPUTransferBufferLocation source{};
		source.transfer_buffer = appstate.transfer_buffer.get();
		source.offset = 0;

		SDL_GPUBufferRegion destination{};
		destination.buffer = appstate.vertex_buffer.get();
		destination.offset = 0;
		destination.size = size;

		SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);

		SDL_EndGPUCopyPass(copy_pass);

		return true;
	}
}  // namespace

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	SDL_SetAppMetadata("Movable Triangle", "2.0", "com.faunjyng.sld3labs");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_Init() failed: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto* state{new AppState()};
	*appstate = state;

	UniqueSDLWindow window{SDL_CreateWindow("Movable Triangle", 800, 600, 0),
		SDL_DestroyWindow};
	if (!window) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_CreateWindow() failed: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	UniqueSDLGPUDevice device{
		SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr),
		SDL_DestroyGPUDevice};
	if (!device) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_CreateGPUDevice() failed: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	state->window = std::move(window);
	state->device = std::move(device);

	if (!SDL_ClaimWindowForGPUDevice(state->device.get(),
			state->window.get())) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_ClaimWindowForGPUDevice() failed: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	state->window_claim_manager.device = state->device.get();
	state->window_claim_manager.window = state->window.get();

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		"GPU backend: %s",
		SDL_GetGPUDeviceDriver(state->device.get()));

	state->pipeline = CreateTriangleGraphicsPipeline(state->device.get(),
		state->window.get());
	if (!state->pipeline) return SDL_APP_FAILURE;

	if (!CreateVertexResources(*state)) return SDL_APP_FAILURE;

	if (!state->imgui_manager.Init(state->window.get(), state->device.get()))
		return SDL_APP_FAILURE;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	auto* state{static_cast<AppState*>(appstate)};

	// ImGui
	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Triangle Controls");
	const char* names[3]{"Vertex 0", "Vertex 1", "Vertex 2"};
	for (int i = 0; i < 3; ++i) {
		ImGui::PushID(i);
		if (ImGui::CollapsingHeader(names[i], ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SliderFloat2("Position",
				state->vertices[i].position,
				-10.0f,
				10.0f);
			ImGui::ColorEdit3("Color", state->vertices[i].color);
		}
		ImGui::PopID();
	}
	if (ImGui::Button("Reset")) {
		for (int i = 0; i < 3; ++i) state->vertices[i] = INITIAL_VERTICES[i];
	}
	ImGui::End();

	ImGui::Render();
	ImDrawData* draw_data{ImGui::GetDrawData()};

	// SDL3
	SDL_GPUCommandBuffer* command_buffer{
		SDL_AcquireGPUCommandBuffer(state->device.get())};
	if (!command_buffer) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_AcquireGPUCommandBuffer() failed: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchain_texture{nullptr};

	if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer,
			state->window.get(),
			&swapchain_texture,
			nullptr,
			nullptr)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_WaitAndAcquireGPUSwapchainTexture() failed: %s",
			SDL_GetError());
		SDL_CancelGPUCommandBuffer(command_buffer);
		return SDL_APP_FAILURE;
	}

	if (swapchain_texture) {
		if (!UploadVertices(*state, command_buffer)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"UploadVertices() failed: %s",
				SDL_GetError());
			SDL_CancelGPUCommandBuffer(command_buffer);
			return SDL_APP_FAILURE;
		}

		ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

		SDL_GPUColorTargetInfo target{};
		target.texture = swapchain_texture;
		target.clear_color = {1.0f, 1.0f, 1.0f, 1.0f};
		target.load_op = SDL_GPU_LOADOP_CLEAR;
		target.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* render_pass{
			SDL_BeginGPURenderPass(command_buffer, &target, 1, nullptr)};
		if (!render_pass) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_BeginGPURenderPass() failed: %s",
				SDL_GetError());
			SDL_CancelGPUCommandBuffer(command_buffer);
			return SDL_APP_FAILURE;
		}

		SDL_BindGPUGraphicsPipeline(render_pass, state->pipeline.get());

		SDL_GPUBufferBinding binding{};
		binding.buffer = state->vertex_buffer.get();
		binding.offset = 0;
		SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);

		SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
		ImGui_ImplSDLGPU3_RenderDrawData(draw_data,
			command_buffer,
			render_pass);

		SDL_EndGPURenderPass(render_pass);
	}

	if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_SubmitGPUCommandBuffer() failed: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	(void)appstate;

	ImGui_ImplSDL3_ProcessEvent(event);
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	(void)result;

	delete static_cast<AppState*>(appstate);
}