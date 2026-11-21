#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <string_view>

#include "gpu_resource.h"

class GPUBufferUploader {
public:
	using ErrorHandler = std::function<void(std::string_view)>;

	auto set_device(SDL_GPUDevice* device) -> GPUBufferUploader&
	{
		m_device = device;
		return *this;
	}

	auto set_error_handler(ErrorHandler handler) -> GPUBufferUploader&
	{
		m_error_handler = std::move(handler);
		return *this;
	}

	// Accepts any contiguous range (`vector`, `array`, `span`, C array, etc.).
	// Only a pointer is stored; the data MUST remain valid until `create()` returns.
	// DO NOT pass a temporary range.
	template <std::ranges::contiguous_range R>
		requires std::ranges::sized_range<R>
	auto set_data(const R& range) -> GPUBufferUploader&
	{
		using value_type = std::ranges::range_value_t<R>;
		static_assert(
			std::is_trivially_copyable_v<value_type>,
			"GPU upload requires trivially copyable element type"
		);
		return set_data(
			static_cast<const void*>(std::ranges::data(range)),
			std::ranges::size(range) * sizeof(value_type)
		);
	}

	auto set_data(const void* data, std::size_t size_bytes) -> GPUBufferUploader&
	{
		m_data = data;
		m_size = size_bytes;
		return *this;
	}

	auto set_usage(SDL_GPUBufferUsageFlags usage) -> GPUBufferUploader&
	{
		m_usage = usage;
		return *this;
	}

	// SDL does not provide a way to query the transfer buffer's size, so the
	// caller must ensure that it is large enough to hold the uploaded data.
	auto set_transfer_buffer(SDL_GPUTransferBuffer* transfer_buffer) -> GPUBufferUploader&
	{
		m_transfer_buffer = transfer_buffer;
		return *this;
	}

	// If `true`, `create()` returns only after the GPU has finished copying the data.
	// If `false`, the copy is submitted and `create()` returns immediately.
	auto set_wait_for_completion(bool wait) -> GPUBufferUploader&
	{
		m_wait_for_completion = wait;
		return *this;
	}

	// Returns an empty buffer on failure; errors are reported through the error handler.
	auto create() const -> UniqueGPUBuffer;

private:
	SDL_GPUDevice* m_device{nullptr};
	SDL_GPUBufferUsageFlags m_usage{0};
	SDL_GPUTransferBuffer* m_transfer_buffer{nullptr};
	ErrorHandler m_error_handler{nullptr};
	const void* m_data{nullptr};
	std::size_t m_size{0};
	bool m_wait_for_completion{false};

	auto report(std::string_view message) const -> void;
	auto fail(std::string_view reason) const -> UniqueGPUBuffer;
	auto fail_sdl(std::string_view function) const -> UniqueGPUBuffer;
};