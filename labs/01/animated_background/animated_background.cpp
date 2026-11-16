/**
 * @file animated_background.cpp
 * @author FaunJyng
 * @brief Continuously change the background color over time
 * @version 1.0
 * @date 16/11/2026
 */

#include <algorithm>
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

	struct Timer {
		Uint64 saved_counter{SDL_GetPerformanceCounter()};
		auto GetDeltaTime() -> float {	// in Seconds
			const Uint64 now{SDL_GetPerformanceCounter()};
			const Uint64 frequency{SDL_GetPerformanceFrequency()};
			float delta_time{static_cast<float>(now - saved_counter) /
							 static_cast<float>(frequency)};
			saved_counter = now;
			return delta_time;
		};
	};

	class ColorAnimator {
	private:
		static constexpr float COLOR_CHANGE_SPEED{0.2};	 // color/sec

		float color[3]{1.0f, 1.0f, 0.0f};  // 0 = red, 1 = green, 2 = blue
		int direction{-1};				   // {1, -1}
		int animated_color{0};			   // {0, 1, 2}

	public:
		auto Red() const -> float { return color[0]; }
		auto Green() const -> float { return color[1]; }
		auto Blue() const -> float { return color[2]; }

		void UpdateColor(float delta_time) {
			float delta_value{delta_time * COLOR_CHANGE_SPEED};
			color[animated_color] += direction * delta_value;
			if (direction < 0)
				color[animated_color] = std::max(color[animated_color], 0.0f);
			else
				color[animated_color] = std::min(color[animated_color], 1.0f);
			if (color[animated_color] == 0.0f || color[animated_color] == 1.0f) {
				direction *= -1;
				animated_color = (animated_color + 2) % 3;
			}
		}
	};

	struct AppState {
		std::unique_ptr<SDL_Window, SDLWindowDestroyer> window;
		std::unique_ptr<SDL_GPUDevice, SDLGPUDeviceDestroyer> gpu_device;
		bool window_claimed{false};
		~AppState() {
			if (window_claimed) SDL_ReleaseWindowFromGPUDevice(gpu_device.get(), window.get());
		}
		std::unique_ptr<Timer> timer{std::make_unique<Timer>()};
		std::unique_ptr<ColorAnimator> color_animator{std::make_unique<ColorAnimator>()};
	};
}  // namespace

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
	SDL_SetAppMetadata("Animated Background", "1.0", "com.faunjyng.sld3labs");
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	AppState *state{new AppState()};
	*appstate = state;

	state->window.reset(SDL_CreateWindow("Animated Background", 800, 600, 0));
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
		SDL_Log("SDL_AcquireGPUCommandBuffer() failed: %s", SDL_GetError());
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
		state->color_animator->UpdateColor(state->timer->GetDeltaTime());
		SDL_GPUColorTargetInfo color_target{};
		color_target.texture = swapchain_texture;
		color_target.clear_color = {state->color_animator->Red(),
									state->color_animator->Green(),
									state->color_animator->Blue(),
									1.0f};
		color_target.load_op = SDL_GPU_LOADOP_CLEAR;
		color_target.store_op = SDL_GPU_STOREOP_STORE;
		SDL_GPURenderPass *pass{SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, nullptr)};
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