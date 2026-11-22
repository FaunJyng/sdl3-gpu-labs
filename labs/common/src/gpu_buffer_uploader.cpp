#include "gpu_buffer_uploader.h"

#include <fmt/format.h>

auto GPUBufferUploader::report(std::string_view message) const -> void
{
	if (m_error_handler) m_error_handler(message);
}

auto GPUBufferUploader::fail(std::string_view reason) const -> UniqueGPUBuffer
{
	report(fmt::format("GPUBufferUploader::create() failed: {}", reason));
	return {};
}

auto GPUBufferUploader::fail_sdl(std::string_view function) const -> UniqueGPUBuffer
{
	return fail(fmt::format("{}() failed: {}", function, SDL_GetError()));
}

auto GPUBufferUploader::create() const -> UniqueGPUBuffer
{
	if (!m_device) return fail("Device has not been set.");
	if (!m_data) return fail("Data has not been set.");
	if (m_size == 0) return fail("Size must not be 0.");
	if (m_size > std::numeric_limits<Uint32>::max())
		return fail("Size exceeds the 4 GiB limit of SDL_GPUBufferCreateInfo.");
	if (m_usage == 0) return fail("Usage has not been set.");
	if (!m_transfer_buffer) return fail("Transfer buffer has not been set.");

	const auto size{static_cast<Uint32>(m_size)};

	SDL_GPUBufferCreateInfo buffer_info{};
	buffer_info.usage = m_usage;
	buffer_info.size = size;
	UniqueGPUBuffer buffer = make_gpu_buffer(m_device, buffer_info);
	if (!buffer) return fail_sdl("SDL_CreateGPUBuffer");

	void* mapped{SDL_MapGPUTransferBuffer(m_device, m_transfer_buffer, true)};
	if (!mapped) return fail_sdl("SDL_MapGPUTransferBuffer");
	SDL_memcpy(mapped, m_data, size);
	SDL_UnmapGPUTransferBuffer(m_device, m_transfer_buffer);

	SDL_GPUCommandBuffer* command_buffer{SDL_AcquireGPUCommandBuffer(m_device)};
	if (!command_buffer) return fail_sdl("SDL_AcquireGPUCommandBuffer");

	SDL_GPUCopyPass* copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
	if (!copy_pass) {
		auto result{fail_sdl("SDL_BeginGPUCopyPass")};
		SDL_CancelGPUCommandBuffer(command_buffer);
		return result;
	}

	SDL_GPUTransferBufferLocation source{};
	source.transfer_buffer = m_transfer_buffer;
	source.offset = 0;

	SDL_GPUBufferRegion destination{};
	destination.buffer = buffer.get();
	destination.offset = 0;
	destination.size = size;

	SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);
	SDL_EndGPUCopyPass(copy_pass);

	if (m_wait_for_completion) {
		SDL_GPUFence* fence{SDL_SubmitGPUCommandBufferAndAcquireFence(command_buffer)};
		if (!fence) return fail_sdl("SDL_SubmitGPUCommandBufferAndAcquireFence");

		const bool waited{SDL_WaitForGPUFences(m_device, true, &fence, 1)};
		if (!waited) {
			auto result{fail_sdl("SDL_WaitForGPUFences")};
			SDL_ReleaseGPUFence(m_device, fence);
			return result;
		}
		SDL_ReleaseGPUFence(m_device, fence);
	} else if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
		return fail_sdl("SDL_SubmitGPUCommandBuffer");
	}

	return buffer;
}