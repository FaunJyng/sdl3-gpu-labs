#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <filesystem>
#include <memory>

namespace {
	namespace fs = std::filesystem;
	template <typename... Args>
	auto GetAbsolutePathRelativeToExe(std::string *output, Args &&...paths) -> bool {
		const char *basePath{SDL_GetBasePath()};
		if (basePath == nullptr) return false;
		fs::path fullPath{fs::path(basePath)};
		((fullPath /= std::forward<Args>(paths)), ...);
		*output = fullPath.string();
		return true;
	}

	auto LoadShader(SDL_GPUDevice *device, const char *path, SDL_GPUShaderStage stage)
		-> SDL_GPUShader * {
		size_t codeSize{0};
		void *code{SDL_LoadFile(path, &codeSize)};
		if (!code) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_LoadFile() failed (%s): %s",
						 path,
						 SDL_GetError());
			return nullptr;
		}

		SDL_GPUShaderCreateInfo info{};
		info.code_size = codeSize;
		info.code = static_cast<const Uint8 *>(code);
		info.entrypoint = "main";
		info.format = SDL_GPU_SHADERFORMAT_SPIRV;
		info.stage = stage;

		SDL_GPUShader *shader{SDL_CreateGPUShader(device, &info)};

		SDL_free(code);	 // !!!!!!!!!!!!!!!!!!!!!!!!!!!

		if (!shader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
						 "SDL_CreateGPUShader() failed (%s): %s",
						 path,
						 SDL_GetError());
		}

		return shader;
	}

	auto CreateGradientGraphicsPipeline(SDL_GPUDevice *device, SDL_Window *window)
		-> SDL_GPUGraphicsPipeline * {
		std::string gradVertPath, gradFragPath;
		if (!GetAbsolutePathRelativeToExe(&gradVertPath,
										  "01",
										  "gradient_background",
										  "shaders",
										  "gradient.vert.spv") ||
			!GetAbsolutePathRelativeToExe(&gradFragPath,
										  "01",
										  "gradient_background",
										  "shaders",
										  "gradient.frag.spv")) {
			return nullptr;
		}

		SDL_GPUShader *vertex_shader{
			LoadShader(device, gradVertPath.data(), SDL_GPU_SHADERSTAGE_VERTEX)};
		if (!vertex_shader) return nullptr;

		SDL_GPUShader *fragment_shader{
			LoadShader(device, gradFragPath.data(), SDL_GPU_SHADERSTAGE_FRAGMENT)};
		if (!fragment_shader) {
			SDL_ReleaseGPUShader(device, vertex_shader);
			return nullptr;
		}

		SDL_GPUColorTargetDescription color_target{};
		color_target.format = SDL_GetGPUSwapchainTextureFormat(device, window);

		SDL_GPUGraphicsPipelineCreateInfo info{};

		info.vertex_shader = vertex_shader;
		info.fragment_shader = fragment_shader;

		info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

		info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;

		info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

		info.target_info.color_target_descriptions = &color_target;
		info.target_info.num_color_targets = 1;
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
		std::unique_ptr<SDL_GPUDevice, SDLGPUDeviceDestroyer> gpuDevice;
		// Pipeline must be released before GPU device
		SDL_GPUGraphicsPipeline *pipeline{nullptr};
		bool windowClaimed{false};

		~AppState() {
			if (pipeline && gpuDevice) {
				SDL_ReleaseGPUGraphicsPipeline(gpuDevice.get(), pipeline);
			}
			if (windowClaimed && gpuDevice && window) {
				SDL_ReleaseWindowFromGPUDevice(gpuDevice.get(), window.get());
			}
		}
	};
}  // namespace

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
	SDL_SetAppMetadata("Gradient Background", "1.0", "com.faunjyng.sld3labs");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto *state{new AppState()};
	*appstate = state;

	state->window.reset(SDL_CreateWindow("Gradient Background", 800, 600, 0));
	if (!state->window) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	state->gpuDevice.reset(SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr));
	if (!state->gpuDevice) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					 "SDL_CreateGPUDevice() failed: %s",
					 SDL_GetError());
		return SDL_APP_FAILURE;
	}
	if (!SDL_ClaimWindowForGPUDevice(state->gpuDevice.get(), state->window.get())) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					 "SDL_ClaimWindowForGPUDevice() failed: %s",
					 SDL_GetError());
		return SDL_APP_FAILURE;
	}
	state->windowClaimed = true;

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
				"GPU backend: %s",
				SDL_GetGPUDeviceDriver(state->gpuDevice.get()));

	state->pipeline = CreateGradientGraphicsPipeline(state->gpuDevice.get(), state->window.get());
	if (!state->pipeline) return SDL_APP_FAILURE;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	auto *state{static_cast<AppState *>(appstate)};

	SDL_GPUCommandBuffer *cmd_buf{SDL_AcquireGPUCommandBuffer(state->gpuDevice.get())};
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