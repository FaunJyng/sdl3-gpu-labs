#pragma once

#include <SDL3/SDL.h>

/*
 * Declared before GPU resources in AppState.
 * Reverse member-destruction order therefore releases pipelines/buffers
 * before releasing the window claim, and destroys the device afterwards.
 */
class WindowClaimManger {
private:
	SDL_GPUDevice* m_device{nullptr};
	SDL_Window* m_window{nullptr};

public:
	WindowClaimManger() = default;

	WindowClaimManger(const WindowClaimManger&) = delete;
	auto operator=(const WindowClaimManger&) -> WindowClaimManger& = delete;
	WindowClaimManger(WindowClaimManger&&) = delete;
	auto operator=(WindowClaimManger&&) -> WindowClaimManger& = delete;

	~WindowClaimManger()
	{
		if (m_device && m_window) SDL_ReleaseWindowFromGPUDevice(m_device, m_window);
	}

	auto set_window(SDL_Window* window) -> void { m_window = window; }
	auto set_device(SDL_GPUDevice* device) -> void { m_device = device; }
};