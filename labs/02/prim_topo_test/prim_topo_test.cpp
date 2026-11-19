/**
 * @file prim_topo_test.cpp
 * @author FaunJyng
 * @brief Experiment with the primitive topologies supported by SDL3.
 * @version 1.0
 * @date 18/11/2026
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
#include <string>
#include <utility>

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

	auto CreateBuffer(SDL_GPUDevice* device, SDL_GPUBufferUsageFlags usage,
		const void* data, Uint32 size) -> UniqueGPUBuffer {
		if (!device || !data || size == 0 || usage == 0) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"CreateBuffer(): invalid device, usage, data, or size");
			return {};
		}

		SDL_GPUBufferCreateInfo buffer_info{};
		buffer_info.usage = usage;
		buffer_info.size = size;

		UniqueGPUBuffer buffer{SDL_CreateGPUBuffer(device, &buffer_info),
			GPUDResourceCleaner<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>{device}};

		if (!buffer) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_CreateGPUBuffer() failed: %s",
				SDL_GetError());
			return {};
		}

		SDL_GPUTransferBufferCreateInfo transfer_info{};
		transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transfer_info.size = size;

		UniqueGPUTransferBuffer transfer_buffer{
			SDL_CreateGPUTransferBuffer(device, &transfer_info),
			GPUDResourceCleaner<SDL_GPUTransferBuffer,
				SDL_ReleaseGPUTransferBuffer>{device}};

		if (!transfer_buffer) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_CreateGPUTransferBuffer() failed: %s",
				SDL_GetError());
			return {};
		}

		void* mapped{
			SDL_MapGPUTransferBuffer(device, transfer_buffer.get(), false)};

		if (!mapped) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_MapGPUTransferBuffer() failed: %s",
				SDL_GetError());
			return {};
		}

		SDL_memcpy(mapped, data, size);
		SDL_UnmapGPUTransferBuffer(device, transfer_buffer.get());

		SDL_GPUCommandBuffer* command_buffer =
			SDL_AcquireGPUCommandBuffer(device);

		if (!command_buffer) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_AcquireGPUCommandBuffer() failed: %s",
				SDL_GetError());
			return {};
		}

		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
		if (!copy_pass) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_BeginGPUCopyPass() failed: %s",
				SDL_GetError());
			SDL_CancelGPUCommandBuffer(command_buffer);
			return {};
		}

		SDL_GPUTransferBufferLocation source{};
		source.transfer_buffer = transfer_buffer.get();
		source.offset = 0;

		SDL_GPUBufferRegion destination{};
		destination.buffer = buffer.get();
		destination.offset = 0;
		destination.size = size;

		SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);

		SDL_EndGPUCopyPass(copy_pass);

		if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_SubmitGPUCommandBuffer() failed: %s",
				SDL_GetError());
			return {};
		}

		return buffer;
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

	auto CreateGraphicsPipeline(SDL_GPUDevice* device, SDL_Window* window,
		SDL_GPUShader* vertex_shader, SDL_GPUShader* fragment_shader,
		SDL_GPUPrimitiveType primitive_type) -> UniqueGPUGraphicsPipeline {
		if (!device || !window || !vertex_shader || !fragment_shader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"CreateGraphicsPipeline(): invalid argument");
			return {};
		}

		const SDL_GPUTextureFormat swapchain_format{
			SDL_GetGPUSwapchainTextureFormat(device, window)};
		if (swapchain_format == SDL_GPU_TEXTUREFORMAT_INVALID) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_GetGPUSwapchainTextureFormat() returned an invalid "
				"format");
			return {};
		}

		SDL_GPUColorTargetDescription color_target{};
		color_target.format = swapchain_format;

		SDL_GPUVertexBufferDescription vertex_buffer_desc{};
		vertex_buffer_desc.slot = 0;
		vertex_buffer_desc.pitch = sizeof(Vertex);
		vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
		vertex_buffer_desc.instance_step_rate = 0;

		SDL_GPUVertexAttribute vertex_attributes[2]{};
		vertex_attributes[0].buffer_slot = 0;
		vertex_attributes[0].location = 0;
		vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		vertex_attributes[0].offset = offsetof(Vertex, position);

		vertex_attributes[1].buffer_slot = 0;
		vertex_attributes[1].location = 1;
		vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertex_attributes[1].offset = offsetof(Vertex, color);

		SDL_GPUGraphicsPipelineCreateInfo info{};
		info.vertex_shader = vertex_shader;
		info.fragment_shader = fragment_shader;
		info.primitive_type = primitive_type;

		info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;

		info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

		info.target_info.color_target_descriptions = &color_target;
		info.target_info.num_color_targets = 1;

		info.vertex_input_state.vertex_buffer_descriptions =
			&vertex_buffer_desc;
		info.vertex_input_state.num_vertex_buffers = 1;
		info.vertex_input_state.vertex_attributes = vertex_attributes;
		info.vertex_input_state.num_vertex_attributes = 2;

		SDL_GPUGraphicsPipeline* pipeline{
			SDL_CreateGPUGraphicsPipeline(device, &info)};
		if (!pipeline) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"SDL_CreateGPUGraphicsPipeline() failed: %s",
				SDL_GetError());
			return {};
		}

		return UniqueGPUGraphicsPipeline{pipeline,
			GPUDResourceCleaner<SDL_GPUGraphicsPipeline,
				SDL_ReleaseGPUGraphicsPipeline>{device}};
	}

	constexpr Vertex VERTICES[6]{
		{{-0.8f, -0.6f}, {1.0f, 0.0f, 0.0f}},
		{{-0.8f, 0.6f}, {0.0f, 1.0f, 0.0f}},
		{{0.0f, -0.6f}, {0.0f, 0.0f, 1.0f}},
		{{0.0f, 0.6f}, {1.0f, 0.0f, 0.0f}},
		{{0.8f, -0.6f}, {0.0f, 1.0f, 0.0f}},
		{{0.8f, 0.6f}, {0.0f, 0.0f, 1.0f}},
	};

	struct AppState {
		UniqueSDLWindow window{nullptr, SDL_DestroyWindow};
		UniqueSDLGPUDevice device{nullptr, SDL_DestroyGPUDevice};
		// Destroyed after GPU resources, but before device.

		WindowClaimManger window_claim_manager;

		UniqueGPUGraphicsPipeline tri_list_pipeline;
		UniqueGPUGraphicsPipeline tri_strip_pipeline;
		UniqueGPUGraphicsPipeline line_list_pipeline;
		UniqueGPUGraphicsPipeline line_strip_pipeline;

		UniqueGPUBuffer vertex_buffer;

		ImGuiManager imgui_manager;

		SDL_GPUPrimitiveType choosen_primitive_type{
			SDL_GPU_PRIMITIVETYPE_TRIANGLELIST};

		~AppState() {
			if (device) SDL_WaitForGPUIdle(device.get());
		}
	};
}  // namespace

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	(void)argc;
	(void)argv;

	SDL_SetAppMetadata("Primitive Topology Test",
		"1.0",
		"com.faunjyng.sdl3labs");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_Init() failed: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto* state{new AppState{}};
	*appstate = state;

	UniqueSDLWindow window{
		SDL_CreateWindow("Primitive Topology Test", 900, 700, 0),
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

	std::string vertex_shader_path;
	std::string fragment_shader_path;
	if (!GetAbsolutePathRelativeToExe(&vertex_shader_path,
			"02",
			"prim_topo_test",
			"shaders",
			"triangle.vert.spv") ||
		!GetAbsolutePathRelativeToExe(&fragment_shader_path,
			"02",
			"prim_topo_test",
			"shaders",
			"triangle.frag.spv")) {
		return SDL_APP_FAILURE;
	}

	UniqueGPUShader vertex_shader{LoadShader(state->device.get(),
		vertex_shader_path.c_str(),
		SDL_GPU_SHADERSTAGE_VERTEX)};
	if (!vertex_shader) return SDL_APP_FAILURE;

	UniqueGPUShader fragment_shader{LoadShader(state->device.get(),
		fragment_shader_path.c_str(),
		SDL_GPU_SHADERSTAGE_FRAGMENT)};
	if (!fragment_shader) return SDL_APP_FAILURE;

	state->tri_list_pipeline = CreateGraphicsPipeline(state->device.get(),
		state->window.get(),
		vertex_shader.get(),
		fragment_shader.get(),
		SDL_GPU_PRIMITIVETYPE_TRIANGLELIST);
	if (!state->tri_list_pipeline) return SDL_APP_FAILURE;

	state->tri_strip_pipeline = CreateGraphicsPipeline(state->device.get(),
		state->window.get(),
		vertex_shader.get(),
		fragment_shader.get(),
		SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP);
	if (!state->tri_strip_pipeline) return SDL_APP_FAILURE;

	state->line_list_pipeline = CreateGraphicsPipeline(state->device.get(),
		state->window.get(),
		vertex_shader.get(),
		fragment_shader.get(),
		SDL_GPU_PRIMITIVETYPE_LINELIST);
	if (!state->line_list_pipeline) return SDL_APP_FAILURE;

	state->line_strip_pipeline = CreateGraphicsPipeline(state->device.get(),
		state->window.get(),
		vertex_shader.get(),
		fragment_shader.get(),
		SDL_GPU_PRIMITIVETYPE_LINESTRIP);
	if (!state->line_strip_pipeline) return SDL_APP_FAILURE;

	state->vertex_buffer = CreateBuffer(state->device.get(),
		SDL_GPU_BUFFERUSAGE_VERTEX,
		VERTICES,
		static_cast<Uint32>(sizeof(VERTICES)));
	if (!state->vertex_buffer) return SDL_APP_FAILURE;

	if (!state->imgui_manager.Init(state->window.get(), state->device.get()))
		return SDL_APP_FAILURE;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	auto* state = static_cast<AppState*>(appstate);

	// ImGui
	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Primitive Topology");
	if (ImGui::RadioButton("Triangle List",
			state->choosen_primitive_type ==
				SDL_GPU_PRIMITIVETYPE_TRIANGLELIST))
		state->choosen_primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	if (ImGui::RadioButton("Triangle Strip",
			state->choosen_primitive_type ==
				SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP))
		state->choosen_primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
	if (ImGui::RadioButton("Line List",
			state->choosen_primitive_type == SDL_GPU_PRIMITIVETYPE_LINELIST))
		state->choosen_primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
	if (ImGui::RadioButton("Line Strip",
			state->choosen_primitive_type == SDL_GPU_PRIMITIVETYPE_LINESTRIP))
		state->choosen_primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP;
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

		switch (state->choosen_primitive_type) {
			case SDL_GPU_PRIMITIVETYPE_TRIANGLELIST:
				SDL_BindGPUGraphicsPipeline(render_pass,
					state->tri_list_pipeline.get());
				break;

			case SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP:
				SDL_BindGPUGraphicsPipeline(render_pass,
					state->tri_strip_pipeline.get());
				break;

			case SDL_GPU_PRIMITIVETYPE_LINELIST:
				SDL_BindGPUGraphicsPipeline(render_pass,
					state->line_list_pipeline.get());
				break;

			case SDL_GPU_PRIMITIVETYPE_LINESTRIP:
				SDL_BindGPUGraphicsPipeline(render_pass,
					state->line_strip_pipeline.get());
				break;

			default:
				SDL_BindGPUGraphicsPipeline(render_pass,
					state->tri_list_pipeline.get());
		}

		SDL_GPUBufferBinding vertex_binding{};
		vertex_binding.buffer = state->vertex_buffer.get();
		vertex_binding.offset = 0;

		SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

		SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);

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
