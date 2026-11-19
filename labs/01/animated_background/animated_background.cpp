/**
 * @file animated_background.cpp
 * @author FaunJyng
 * @brief Continuously change the background color over time
 * @version 2.0
 * @date 16/11/2026
 */
#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>

namespace {
	using UniqueSDLWindow =
		std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

	using UniqueSDLGPUDevice =
		std::unique_ptr<SDL_GPUDevice, decltype(&SDL_DestroyGPUDevice)>;

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
	using UniqueTimer = std::unique_ptr<Timer>;

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
			if (color[animated_color] == 0.0f ||
				color[animated_color] == 1.0f) {
				direction *= -1;
				animated_color = (animated_color + 2) % 3;
			}
		}
	};
	using UniqueColorAnimator = std::unique_ptr<ColorAnimator>;

	struct AppState {
		UniqueSDLWindow window{nullptr, SDL_DestroyWindow};
		UniqueSDLGPUDevice device{nullptr, SDL_DestroyGPUDevice};
		// Destroyed after GPU resources, but before device.

		WindowClaimManger window_claim_manager;

		UniqueTimer timer{std::make_unique<Timer>()};
		UniqueColorAnimator color_animator{std::make_unique<ColorAnimator>()};
	};
}  // namespace

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	(void)argc;
	(void)argv;

	SDL_SetAppMetadata("Animated Background", "2.0", "com.faunjyng.sld3labs");
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"SDL_Init() failed: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	AppState* state{new AppState()};
	*appstate = state;

	UniqueSDLWindow window{SDL_CreateWindow("Animated Background", 800, 600, 0),
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

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	AppState* state{static_cast<AppState*>(appstate)};

	SDL_GPUCommandBuffer* command_buffer{
		SDL_AcquireGPUCommandBuffer(state->device.get())};
	if (!command_buffer) {
		SDL_Log("SDL_AcquireGPUCommandBuffer() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchain_texture{nullptr};

	if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer,
			state->window.get(),
			&swapchain_texture,
			nullptr,
			nullptr)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"Accquire swapchain texture failed: %s",
			SDL_GetError());
		SDL_CancelGPUCommandBuffer(command_buffer);
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

		SDL_GPURenderPass* render_pass{
			SDL_BeginGPURenderPass(command_buffer, &color_target, 1, nullptr)};
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

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	(void)result;

	delete static_cast<AppState*>(appstate);
}