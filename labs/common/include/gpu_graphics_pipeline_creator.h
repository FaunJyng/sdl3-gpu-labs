#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <string_view>
#include <utility>
#include <vector>

#include "gpu_resource.h"

class GPUGraphicsPipelineCreator {
public:
	using ErrorHandler = std::function<void(std::string_view)>;

	auto set_error_handler(ErrorHandler handler) -> GPUGraphicsPipelineCreator&
	{
		m_error_handler = std::move(handler);
		return *this;
	}

	auto set_device(SDL_GPUDevice* device) -> GPUGraphicsPipelineCreator&
	{
		m_device = device;
		return *this;
	}

	auto set_vertex_shader(SDL_GPUShader* vertex_shader) -> GPUGraphicsPipelineCreator&
	{
		m_vertex_shader = vertex_shader;
		return *this;
	}

	auto set_fragment_shader(SDL_GPUShader* fragment_shader) -> GPUGraphicsPipelineCreator&
	{
		m_fragment_shader = fragment_shader;
		return *this;
	}

	auto set_primitive_type(SDL_GPUPrimitiveType type) -> GPUGraphicsPipelineCreator&
	{
		m_primitive_type = type;
		return *this;
	}

	auto set_rasterizer_state(const SDL_GPURasterizerState& state) -> GPUGraphicsPipelineCreator&
	{
		m_rasterizer_state = state;
		return *this;
	}

	auto set_multisample_state(const SDL_GPUMultisampleState& state) -> GPUGraphicsPipelineCreator&
	{
		m_multisample_state = state;
		return *this;
	}

	auto set_depth_stencil_state(const SDL_GPUDepthStencilState& state)
		-> GPUGraphicsPipelineCreator&
	{
		m_depth_stencil_state = state;
		return *this;
	}

	auto add_vertex_buffer(const SDL_GPUVertexBufferDescription& desc)
		-> GPUGraphicsPipelineCreator&
	{
		m_vertex_buffer_descriptions.push_back(desc);
		return *this;
	}

	auto add_vertex_attribute(const SDL_GPUVertexAttribute& attr) -> GPUGraphicsPipelineCreator&
	{
		m_vertex_attributes.push_back(attr);
		return *this;
	}

	auto add_color_target(const SDL_GPUColorTargetDescription& desc) -> GPUGraphicsPipelineCreator&
	{
		m_color_target_descriptions.push_back(desc);
		return *this;
	}

	auto set_depth_stencil_target(SDL_GPUTextureFormat format) -> GPUGraphicsPipelineCreator&
	{
		m_depth_stencil_format = format;
		m_has_depth_stencil_target = true;
		return *this;
	}

	// Returns an empty pipeline on failure; errors are reported through the error handler.
	[[nodiscard]] auto create() const -> UniqueGPUGraphicsPipeline;

private:
	ErrorHandler m_error_handler{nullptr};

	SDL_GPUPrimitiveType m_primitive_type{SDL_GPU_PRIMITIVETYPE_TRIANGLELIST};
	SDL_GPURasterizerState m_rasterizer_state{};
	SDL_GPUMultisampleState m_multisample_state{};
	SDL_GPUDepthStencilState m_depth_stencil_state{};

	SDL_GPUDevice* m_device{nullptr};
	SDL_GPUShader* m_vertex_shader{nullptr};
	SDL_GPUShader* m_fragment_shader{nullptr};

	std::vector<SDL_GPUVertexBufferDescription> m_vertex_buffer_descriptions;
	std::vector<SDL_GPUVertexAttribute> m_vertex_attributes;
	std::vector<SDL_GPUColorTargetDescription> m_color_target_descriptions;

	SDL_GPUTextureFormat m_depth_stencil_format{SDL_GPU_TEXTUREFORMAT_INVALID};
	bool m_has_depth_stencil_target{false};

	auto report(std::string_view message) const -> void;
	auto fail(std::string_view reason) const -> UniqueGPUGraphicsPipeline;
	auto fail_sdl(std::string_view function) const -> UniqueGPUGraphicsPipeline;
};