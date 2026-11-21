#pragma once

#include <SDL3/SDL.h>

#include <memory>

// SDL resource not associated with a device: `Fn(resource)`.
template <auto Fn>
struct SDLDeleter {
	template <typename T>
	void operator()(T* resource) const noexcept
	{
		if (resource) Fn(resource);
	}
};
// Note the destruction order: when grouping resources in a class, declare
// `UniqueSDLGPUDevice` BEFORE the GPU resources (members are destroyed in reverse order).
using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLDeleter<SDL_DestroyWindow>>;

// Note the destruction order: when grouping resources in a class, declare
// UniqueSDLGPUDevice BEFORE the GPU resources (members are destroyed in reverse order).
using UniqueSDLGPUDevice = std::unique_ptr<SDL_GPUDevice, SDLDeleter<SDL_DestroyGPUDevice>>;

// Memory allocated by SDL, such as the result of `SDL_LoadFile()`.
using UniqueSDLMemory = std::unique_ptr<void, SDLDeleter<SDL_free>>;

// Resource owned by an `SDL_GPUDevice`: `ReleaseFunction(device, resource)`.
// The device must remain alive while the resource is being released.
template <typename T, auto ReleaseFunction>
struct GPUDeviceResourceCleaner {
	static_assert(
		std::is_invocable_v<decltype(ReleaseFunction), SDL_GPUDevice*, T*>,
		"ReleaseFunction must be callable as ReleaseFunction(SDL_GPUDevice*, T*). "
		"Check that T matches the release function"
	);

	SDL_GPUDevice* device{nullptr};

	void operator()(T* resource) const noexcept
	{
		if (!resource) return;
		SDL_assert(device != nullptr);
		ReleaseFunction(device, resource);
	}
};

template <typename T, auto ReleaseFunction>
using UniqueGPUResource = std::unique_ptr<T, GPUDeviceResourceCleaner<T, ReleaseFunction>>;

using UniqueGPUShader = UniqueGPUResource<SDL_GPUShader, SDL_ReleaseGPUShader>;
using UniqueGPUBuffer = UniqueGPUResource<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>;
using UniqueGPUTransferBuffer =
	UniqueGPUResource<SDL_GPUTransferBuffer, SDL_ReleaseGPUTransferBuffer>;
using UniqueGPUTexture = UniqueGPUResource<SDL_GPUTexture, SDL_ReleaseGPUTexture>;
using UniqueGPUSampler = UniqueGPUResource<SDL_GPUSampler, SDL_ReleaseGPUSampler>;
using UniqueGPUGraphicsPipeline =
	UniqueGPUResource<SDL_GPUGraphicsPipeline, SDL_ReleaseGPUGraphicsPipeline>;
using UniqueGPUComputePipeline =
	UniqueGPUResource<SDL_GPUComputePipeline, SDL_ReleaseGPUComputePipeline>;
using UniqueGPUFence = UniqueGPUResource<SDL_GPUFence, SDL_ReleaseGPUFence>;