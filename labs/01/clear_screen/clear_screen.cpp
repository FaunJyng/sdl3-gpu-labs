/**
 * @file clear_screen.cpp
 * @author FaunJyng
 * @brief Cover the whole screen with an arbitraty color
 * @version 1
 * @date 15/11/2026
 */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>

namespace {
	struct SDLWindowDestroyer {
		void operator()(SDL_Window *window) const {
			if (window) SDL_DestroyWindow(window);
		}
	};

	struct SDLGPUDeviceDestroyer {
		void operator()(SDL_GPUDevice *device) const {
			if (device) SDL_DestroyGPUDevice(device);
		}
	};

	struct AppState {
		std::unique_ptr<SDL_Window, SDLWindowDestroyer> window;
		std::unique_ptr<SDL_GPUDevice, SDLGPUDeviceDestroyer> gpu_device;
		bool window_claimed{false};
		~AppState() {
			if (window_claimed) SDL_ReleaseWindowFromGPUDevice(gpu_device.get(), window.get());
		}
	};
}  // namespace

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
	SDL_SetAppMetadata("Clear Screen", "1.0", "com.faunjyng.sld3labs");
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	AppState *state{new AppState()};
	*appstate = state;

	state->window.reset(SDL_CreateWindow("Clear Screen", 800, 600, 0));
	if (!state->window) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	const SDL_GPUShaderFormat shader_formats{SDL_GPU_SHADERFORMAT_SPIRV};
	state->gpu_device.reset(SDL_CreateGPUDevice(shader_formats, true, nullptr));
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

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	AppState *state{static_cast<AppState *>(appstate)};

	SDL_GPUCommandBuffer *cmd_buf{SDL_AcquireGPUCommandBuffer(state->gpu_device.get())};
	if (!cmd_buf) {
		SDL_Log("SDL_AcquireGPUCommandBuffer(): %s", SDL_GetError());
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
					 "Accquire swapchain texture failed: %s",
					 SDL_GetError());
		SDL_CancelGPUCommandBuffer(cmd_buf);
		return SDL_APP_FAILURE;
	}

	if (swapchain_texture) {
		SDL_GPUColorTargetInfo color_target{};
		color_target.texture = swapchain_texture;
		color_target.clear_color = {0.0f, 0.5f, 1.0f, 1.0f};
		color_target.load_op = SDL_GPU_LOADOP_CLEAR;
		color_target.store_op = SDL_GPU_STOREOP_STORE;
		SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, nullptr);
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