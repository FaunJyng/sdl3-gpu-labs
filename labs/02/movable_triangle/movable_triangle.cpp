/**
 * @file movable_triangle.cpp
 * @author FaunJyng
 * @brief Use ImGui to interactively reposition the triangle's three vertices.
 * @version 1.0
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

	namespace fs = std::filesystem;
	template <typename... Args>
	auto GetAbsolutePathRelativeToExe(std::string *output, Args &&...paths) -> bool {
		const char *base_path{SDL_GetBasePath()};
		if (base_path == nullptr) return false;
		fs::path full_path{fs::path(base_path)};
		((full_path /= std::forward<Args>(paths)), ...);
		*output = full_path.string();
		return true;
	}

	auto LoadShader(SDL_GPUDevice *device, const char *path, SDL_GPUShaderStage stage)
		-> SDL_GPUShader * {
		size_t code_size{0};
		void *code{SDL_LoadFile(path, &code_size)};
		if (!code) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_LoadFile() failed (%s): %s",
						 path,
						 SDL_GetError());
			return nullptr;
		}

		SDL_GPUShaderCreateInfo info{};
		info.code_size = code_size;
		info.code = static_cast<const Uint8 *>(code);
		info.entrypoint = "main";
		info.format = SDL_GPU_SHADERFORMAT_SPIRV;
		info.stage = stage;

		SDL_GPUShader *shader{SDL_CreateGPUShader(device, &info)};

		SDL_free(code);

		if (!shader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_CreateGPUShader() failed (%s): %s",
						 path,
						 SDL_GetError());
		}

		return shader;
	}

	auto CreateTriangleGraphicsPipeline(SDL_GPUDevice *device, SDL_Window *window)
		-> SDL_GPUGraphicsPipeline * {
		std::string tri_vert_path, tri_frag_path;
		if (!GetAbsolutePathRelativeToExe(&tri_vert_path,
										  "02",
										  "vertex_buffer_triangle",
										  "shaders",
										  "triangle.vert.spv") ||
			!GetAbsolutePathRelativeToExe(&tri_frag_path,
										  "02",
										  "vertex_buffer_triangle",
										  "shaders",
										  "triangle.frag.spv")) {
			return nullptr;
		}

		SDL_GPUShader *vertex_shader{
			LoadShader(device, tri_vert_path.data(), SDL_GPU_SHADERSTAGE_VERTEX)};
		if (!vertex_shader) return nullptr;

		SDL_GPUShader *fragment_shader{
			LoadShader(device, tri_frag_path.data(), SDL_GPU_SHADERSTAGE_FRAGMENT)};
		if (!fragment_shader) {
			SDL_ReleaseGPUShader(device, vertex_shader);
			return nullptr;
		}

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
		info.vertex_shader = vertex_shader;
		info.fragment_shader = fragment_shader;

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

		SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);

		SDL_ReleaseGPUShader(device, vertex_shader);
		SDL_ReleaseGPUShader(device, fragment_shader);

		if (!pipeline) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_CreateGPUGraphicsPipeline() failed: %s",
						 SDL_GetError());
		}

		return pipeline;
	}

	struct SDLWindowDestroyer {
		void operator()(SDL_Window *window) const {
			if (window) {
				SDL_DestroyWindow(window);
			}
		}
	};

	struct SDLGPUDeviceDestroyer {
		void operator()(SDL_GPUDevice *device) const {
			if (device) {
				SDL_DestroyGPUDevice(device);
			}
		}
	};

	constexpr Vertex INITIAL_VERTICES[3]{
		{{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
		{{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
	};

	struct AppState {
		std::unique_ptr<SDL_Window, SDLWindowDestroyer> window;
		std::unique_ptr<SDL_GPUDevice, SDLGPUDeviceDestroyer> gpu_device;
		// Pipeline must be released before GPU device
		SDL_GPUGraphicsPipeline *pipeline{nullptr};
		Vertex vertices[3]{INITIAL_VERTICES[0], INITIAL_VERTICES[1], INITIAL_VERTICES[2]};
		SDL_GPUBuffer *vertex_buffer{nullptr};
		SDL_GPUTransferBuffer *transfer_buffer{nullptr};
		bool window_claimed{false};
		bool imgui_initialized{false};

		~AppState() {
			if (gpu_device) {
				SDL_WaitForGPUIdle(gpu_device.get());
			}
			if (imgui_initialized) {
				ImGui_ImplSDL3_Shutdown();
				ImGui_ImplSDLGPU3_Shutdown();
				ImGui::DestroyContext();
			}
			if (transfer_buffer && gpu_device) {
				SDL_ReleaseGPUTransferBuffer(gpu_device.get(), transfer_buffer);
			}
			if (vertex_buffer && gpu_device) {
				SDL_ReleaseGPUBuffer(gpu_device.get(), vertex_buffer);
			}
			if (pipeline && gpu_device) {
				SDL_ReleaseGPUGraphicsPipeline(gpu_device.get(), pipeline);
			}
			if (window_claimed && gpu_device && window) {
				SDL_ReleaseWindowFromGPUDevice(gpu_device.get(), window.get());
			}
		}
	};

	auto CreateVertexResources(AppState &appstate) -> bool {
		const Uint32 size{static_cast<Uint32>(sizeof(appstate.vertices))};

		SDL_GPUBufferCreateInfo buf_info{};
		buf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		buf_info.size = size;
		appstate.vertex_buffer = SDL_CreateGPUBuffer(appstate.gpu_device.get(), &buf_info);
		if (!appstate.vertex_buffer) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_CreateGPUBuffer() failed: %s",
						 SDL_GetError());
			return false;
		}

		SDL_GPUTransferBufferCreateInfo trans_buf_info{};
		trans_buf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		trans_buf_info.size = size;
		appstate.transfer_buffer =
			SDL_CreateGPUTransferBuffer(appstate.gpu_device.get(), &trans_buf_info);
		if (!appstate.transfer_buffer) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_CreateGPUTransferBuffer() failed: %s",
						 SDL_GetError());
			return false;
		}
		return true;
	}

	auto UploadVertices(AppState &appstate, SDL_GPUCommandBuffer *cmd_buf) -> bool {
		SDL_GPUDevice *device{appstate.gpu_device.get()};
		const Uint32 size{static_cast<Uint32>(sizeof(appstate.vertices))};

		// cycle = true: If GPU still reading last frame data, SDL allocate new memory
		void *mapped{SDL_MapGPUTransferBuffer(device, appstate.transfer_buffer, true)};
		if (!mapped) return false;
		SDL_memcpy(mapped, appstate.vertices, size);
		SDL_UnmapGPUTransferBuffer(device, appstate.transfer_buffer);

		SDL_GPUCopyPass *copy{SDL_BeginGPUCopyPass(cmd_buf)};
		if (!copy) return false;

		SDL_GPUTransferBufferLocation src{};
		src.transfer_buffer = appstate.transfer_buffer;
		src.offset = 0;

		SDL_GPUBufferRegion dst{};
		dst.buffer = appstate.vertex_buffer;
		dst.offset = 0;
		dst.size = size;

		SDL_UploadToGPUBuffer(copy, &src, &dst, true);	// cycle = true
		SDL_EndGPUCopyPass(copy);
		return true;
	}
}  // namespace

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
	SDL_SetAppMetadata("Vertex Buffer Triangle", "1.0", "com.faunjyng.sld3labs");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto *state{new AppState()};
	*appstate = state;

	state->window.reset(SDL_CreateWindow("Vertex Buffer Triangle", 800, 600, 0));
	if (!state->window) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	state->gpu_device.reset(SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr));
	if (!state->gpu_device) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					 "SDL_CreateGPUDevice() failed: %s",
					 SDL_GetError());
		return SDL_APP_FAILURE;
	}
	if (!SDL_ClaimWindowForGPUDevice(state->gpu_device.get(), state->window.get())) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					 "SDL_ClaimWindowForGPUDevice() failed: %s",
					 SDL_GetError());
		return SDL_APP_FAILURE;
	}
	state->window_claimed = true;

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
				"GPU backend: %s",
				SDL_GetGPUDeviceDriver(state->gpu_device.get()));

	state->pipeline = CreateTriangleGraphicsPipeline(state->gpu_device.get(), state->window.get());
	if (!state->pipeline) return SDL_APP_FAILURE;

	if (!CreateVertexResources(*state)) return SDL_APP_FAILURE;

	// ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io{ImGui::GetIO()};
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsLight();

	ImGui_ImplSDL3_InitForSDLGPU(state->window.get());

	ImGui_ImplSDLGPU3_InitInfo init_info{};
	init_info.Device = state->gpu_device.get();
	init_info.ColorTargetFormat =
		SDL_GetGPUSwapchainTextureFormat(state->gpu_device.get(), state->window.get());
	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	ImGui_ImplSDLGPU3_Init(&init_info);

	state->imgui_initialized = true;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	auto *state{static_cast<AppState *>(appstate)};

	// ImGui
	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Triangle Controls");
	const char *names[3]{"Vertex 0", "Vertex 1", "Vertex 2"};
	for (int i = 0; i < 3; ++i) {
		ImGui::PushID(i);
		if (ImGui::CollapsingHeader(names[i], ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SliderFloat2("Position", state->vertices[i].position, -10.0f, 10.0f);
			ImGui::ColorEdit3("Color", state->vertices[i].color);
		}
		ImGui::PopID();
	}
	if (ImGui::Button("Reset")) {
		for (int i = 0; i < 3; ++i) state->vertices[i] = INITIAL_VERTICES[i];
	}
	ImGui::End();

	ImGui::Render();
	ImDrawData *draw_data{ImGui::GetDrawData()};

	// SDL3
	SDL_GPUCommandBuffer *cmd_buf{SDL_AcquireGPUCommandBuffer(state->gpu_device.get())};
	if (!cmd_buf) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					 "SDL_AcquireGPUCommandBuffer() failed: %s",
					 SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture *swapchain_texture{nullptr};
	Uint32 width{0};
	Uint32 height{0};
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf,
											   state->window.get(),
											   &swapchain_texture,
											   &width,
											   &height)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					 "SDL_WaitAndAcquireGPUSwapchainTexture() failed: %s",
					 SDL_GetError());
		SDL_CancelGPUCommandBuffer(cmd_buf);
		return SDL_APP_FAILURE;
	}

	if (swapchain_texture) {
		if (!UploadVertices(*state, cmd_buf)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "UploadVertices() failed: %s",
						 SDL_GetError());
			SDL_CancelGPUCommandBuffer(cmd_buf);
			return SDL_APP_FAILURE;
		}

		ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buf);

		SDL_GPUColorTargetInfo target{};
		target.texture = swapchain_texture;
		target.clear_color = {1.0f, 1.0f, 1.0f, 1.0f};
		target.load_op = SDL_GPU_LOADOP_CLEAR;
		target.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass *pass{SDL_BeginGPURenderPass(cmd_buf, &target, 1, nullptr)};
		if (!pass) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_BeginGPURenderPass() failed: %s",
						 SDL_GetError());
			return SDL_APP_FAILURE;
		}

		SDL_BindGPUGraphicsPipeline(pass, state->pipeline);

		SDL_GPUBufferBinding binding{};
		binding.buffer = state->vertex_buffer;
		binding.offset = 0;
		SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);

		SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
		ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd_buf, pass);

		SDL_EndGPURenderPass(pass);
	}

	if (!SDL_SubmitGPUCommandBuffer(cmd_buf)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					 "SDL_SubmitGPUCommandBuffer() failed: %s",
					 SDL_GetError());
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	ImGui_ImplSDL3_ProcessEvent(event);
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	if (appstate) {
		AppState *state{static_cast<AppState *>(appstate)};
		delete state;
	}
}