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

template <typename UniqueResource>
[[nodiscard]] auto adopt(SDL_GPUDevice* device, typename UniqueResource::pointer resource)
	-> UniqueResource
{
	return UniqueResource{resource, typename UniqueResource::deleter_type{device}};
}

template <typename UniqueResource>
[[nodiscard]] auto adopt_checked(
	SDL_GPUDevice* device,
	typename UniqueResource::pointer resource,
	const char* function_name
) -> UniqueResource
{
	if (!resource) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"[ERROR] %s() failed: %s",
			function_name,
			SDL_GetError()
		);
	}
	return adopt<UniqueResource>(device, resource);
}

[[nodiscard]] inline auto
make_gpu_shader(SDL_GPUDevice* device, const SDL_GPUShaderCreateInfo& info) -> UniqueGPUShader
{
	return adopt_checked<UniqueGPUShader>(
		device,
		SDL_CreateGPUShader(device, &info),
		"SDL_CreateGPUShader"
	);
}

[[nodiscard]] inline auto
make_gpu_buffer(SDL_GPUDevice* device, const SDL_GPUBufferCreateInfo& info) -> UniqueGPUBuffer
{
	return adopt_checked<UniqueGPUBuffer>(
		device,
		SDL_CreateGPUBuffer(device, &info),
		"SDL_CreateGPUBuffer"
	);
}

[[nodiscard]] inline auto make_gpu_transfer_buffer(
	SDL_GPUDevice* device,
	const SDL_GPUTransferBufferCreateInfo& info
) -> UniqueGPUTransferBuffer
{
	return adopt_checked<UniqueGPUTransferBuffer>(
		device,
		SDL_CreateGPUTransferBuffer(device, &info),
		"SDL_CreateGPUTransferBuffer"
	);
}

[[nodiscard]] inline auto
make_gpu_texture(SDL_GPUDevice* device, const SDL_GPUTextureCreateInfo& info) -> UniqueGPUTexture
{
	return adopt_checked<UniqueGPUTexture>(
		device,
		SDL_CreateGPUTexture(device, &info),
		"SDL_CreateGPUTexture"
	);
}

[[nodiscard]] inline auto
make_gpu_sampler(SDL_GPUDevice* device, const SDL_GPUSamplerCreateInfo& info) -> UniqueGPUSampler
{
	return adopt_checked<UniqueGPUSampler>(
		device,
		SDL_CreateGPUSampler(device, &info),
		"SDL_CreateGPUSampler"
	);
}

[[nodiscard]] inline auto make_gpu_graphics_pipeline(
	SDL_GPUDevice* device,
	const SDL_GPUGraphicsPipelineCreateInfo& info
) -> UniqueGPUGraphicsPipeline
{
	return adopt_checked<UniqueGPUGraphicsPipeline>(
		device,
		SDL_CreateGPUGraphicsPipeline(device, &info),
		"SDL_CreateGPUGraphicsPipeline"
	);
}

[[nodiscard]] inline auto make_gpu_compute_pipeline(
	SDL_GPUDevice* device,
	const SDL_GPUComputePipelineCreateInfo& info
) -> UniqueGPUComputePipeline
{
	return adopt_checked<UniqueGPUComputePipeline>(
		device,
		SDL_CreateGPUComputePipeline(device, &info),
		"SDL_CreateGPUComputePipeline"
	);
}

[[nodiscard]] inline auto adopt_fence(SDL_GPUDevice* device, SDL_GPUFence* fence) -> UniqueGPUFence
{
	return adopt<UniqueGPUFence>(device, fence);
}
