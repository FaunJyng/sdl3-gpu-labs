/**
 * @file vertex_buffer_triangle.cpp
 * @author FaunJyng
 * @brief Store three vertices in an `SDL_GPUBuffer` and use it to render a triangle.
 * @version 1.0
 * @date 16/11/2026
 */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstddef>
#include <filesystem>
#include <memory>

namespace {
	struct Vertex {
		float position[2];
		float color[3];
	};
	constexpr Vertex VERTICES[3]{
		{{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
		{{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
	};

	auto CreateVertexBuffer(SDL_GPUDevice *device) -> SDL_GPUBuffer * {
		const Uint32 size{static_cast<Uint32>(sizeof(VERTICES))};

		SDL_GPUBufferCreateInfo buf_info{};
		buf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		buf_info.size = size;
		SDL_GPUBuffer *buf{SDL_CreateGPUBuffer(device, &buf_info)};
		if (!buf) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_CreateGPUBuffer() failed: %s",
						 SDL_GetError());
			return nullptr;
		}

		SDL_GPUTransferBufferCreateInfo trans_buf_info{};
		trans_buf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		trans_buf_info.size = size;
		SDL_GPUTransferBuffer *trans_buf{SDL_CreateGPUTransferBuffer(device, &trans_buf_info)};
		if (!trans_buf) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_CreateGPUTransferBuffer() failed: %s",
						 SDL_GetError());
			SDL_ReleaseGPUBuffer(device, buf);
			return nullptr;
		}

		void *mapped{SDL_MapGPUTransferBuffer(device, trans_buf, false)};
		if (!mapped) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_MapGPUTransferBuffer() failed: %s",
						 SDL_GetError());
			SDL_ReleaseGPUTransferBuffer(device, trans_buf);
			SDL_ReleaseGPUBuffer(device, buf);
			return nullptr;
		}
		SDL_memcpy(mapped, VERTICES, size);
		SDL_UnmapGPUTransferBuffer(device, trans_buf);

		SDL_GPUCommandBuffer *cmd{SDL_AcquireGPUCommandBuffer(device)};
		SDL_GPUCopyPass *copy{cmd ? SDL_BeginGPUCopyPass(cmd) : nullptr};
		if (!copy) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "Begin copy pass failed: %s",
						 SDL_GetError());
			if (cmd) SDL_CancelGPUCommandBuffer(cmd);
			SDL_ReleaseGPUTransferBuffer(device, trans_buf);
			SDL_ReleaseGPUBuffer(device, buf);
			return nullptr;
		}

		SDL_GPUTransferBufferLocation src{};
		src.transfer_buffer = trans_buf;
		src.offset = 0;

		SDL_GPUBufferRegion dst{};
		dst.buffer = buf;
		dst.offset = 0;
		dst.size = size;

		SDL_UploadToGPUBuffer(copy, &src, &dst, false);
		SDL_EndGPUCopyPass(copy);

		bool ok{SDL_SubmitGPUCommandBuffer(cmd)};
		SDL_ReleaseGPUTransferBuffer(device, trans_buf);

		if (!ok) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_SubmitGPUCommandBuffer() failed: %s",
						 SDL_GetError());
			SDL_ReleaseGPUBuffer(device, buf);
			return nullptr;
		}

		return buf;
	}

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

	struct AppState {
		std::unique_ptr<SDL_Window, SDLWindowDestroyer> window;
		std::unique_ptr<SDL_GPUDevice, SDLGPUDeviceDestroyer> gpu_device;
		// Pipeline must be released before GPU device
		SDL_GPUGraphicsPipeline *pipeline{nullptr};
		SDL_GPUBuffer *vertex_buffer{nullptr};
		bool window_claimed{false};

		~AppState() {
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

	state->vertex_buffer = CreateVertexBuffer(state->gpu_device.get());
	if (!state->vertex_buffer) return SDL_APP_FAILURE;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	auto *state{static_cast<AppState *>(appstate)};

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