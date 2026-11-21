#include "gpu_graphics_pipeline_creator.h"

#include <fmt/format.h>

auto GPUGraphicsPipelineCreator::report(std::string_view message) const -> void
{
	if (m_error_handler) m_error_handler(message);
}

auto GPUGraphicsPipelineCreator::fail(std::string_view reason) const -> UniqueGPUGraphicsPipeline
{
	report(fmt::format("[ERROR] GPUGraphicsPipelineCreator::create() failed: {}", reason));
	return {};
}

auto GPUGraphicsPipelineCreator::fail_sdl(std::string_view function) const
	-> UniqueGPUGraphicsPipeline
{
	return fail(fmt::format("{}() failed: {}", function, SDL_GetError()));
}

auto GPUGraphicsPipelineCreator::create() const -> UniqueGPUGraphicsPipeline
{
	if (!m_device) return fail("Device has not been set.");
	if (!m_vertex_shader) return fail("Vertex shader has not been set.");
	if (!m_fragment_shader) return fail("Fragment shader has not been set.");

	if (m_color_target_descriptions.empty() && !m_has_depth_stencil_target)
		return fail("No render target has been set.");

	if (m_has_depth_stencil_target && m_depth_stencil_format == SDL_GPU_TEXTUREFORMAT_INVALID)
		return fail("Depth-stencil target enabled but format is invalid.");

	for (const auto& attr : m_vertex_attributes) {
		if (attr.buffer_slot >= m_vertex_buffer_descriptions.size())
			return fail(fmt::format(
				"Vertex attribute at location {} references buffer slot {}, "
				"which has no description.",
				attr.location,
				attr.buffer_slot
			));
	}

	SDL_GPUGraphicsPipelineCreateInfo info{};
	info.vertex_shader = m_vertex_shader;
	info.fragment_shader = m_fragment_shader;

	info.vertex_input_state.vertex_buffer_descriptions = m_vertex_buffer_descriptions.data();
	info.vertex_input_state.num_vertex_buffers =
		static_cast<Uint32>(m_vertex_buffer_descriptions.size());
	info.vertex_input_state.vertex_attributes = m_vertex_attributes.data();
	info.vertex_input_state.num_vertex_attributes = static_cast<Uint32>(m_vertex_attributes.size());

	info.primitive_type = m_primitive_type;
	info.rasterizer_state = m_rasterizer_state;
	info.multisample_state = m_multisample_state;
	info.depth_stencil_state = m_depth_stencil_state;

	info.target_info.color_target_descriptions = m_color_target_descriptions.data();
	info.target_info.num_color_targets = static_cast<Uint32>(m_color_target_descriptions.size());
	info.target_info.depth_stencil_format = m_depth_stencil_format;
	info.target_info.has_depth_stencil_target = m_has_depth_stencil_target;

	UniqueGPUGraphicsPipeline pipeline{SDL_CreateGPUGraphicsPipeline(m_device, &info), {m_device}};
	if (!pipeline) return fail_sdl("SDL_CreateGPUGraphicsPipeline");

	return pipeline;
}