#include "gpu_graphics_pipeline_creator.h"

#include <fmt/format.h>

auto GPUGraphicsPipelineCreator::report(std::string_view message) const -> void
{
	if (m_error_handler) m_error_handler(message);
}

auto GPUGraphicsPipelineCreator::fail(std::string_view reason) const -> UniqueGPUGraphicsPipeline
{
	report(fmt::format("GPUGraphicsPipelineCreator::create() failed: {}", reason));
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

	SDL_GPUGraphicsPipelineCreateInfo graphics_pipeline_create_info{};
	graphics_pipeline_create_info.vertex_shader = m_vertex_shader;
	graphics_pipeline_create_info.fragment_shader = m_fragment_shader;

	graphics_pipeline_create_info.vertex_input_state.vertex_buffer_descriptions =
		m_vertex_buffer_descriptions.data();
	graphics_pipeline_create_info.vertex_input_state.num_vertex_buffers =
		static_cast<Uint32>(m_vertex_buffer_descriptions.size());
	graphics_pipeline_create_info.vertex_input_state.vertex_attributes = m_vertex_attributes.data();
	graphics_pipeline_create_info.vertex_input_state.num_vertex_attributes =
		static_cast<Uint32>(m_vertex_attributes.size());

	graphics_pipeline_create_info.primitive_type = m_primitive_type;
	graphics_pipeline_create_info.rasterizer_state = m_rasterizer_state;
	graphics_pipeline_create_info.multisample_state = m_multisample_state;
	graphics_pipeline_create_info.depth_stencil_state = m_depth_stencil_state;

	graphics_pipeline_create_info.target_info.color_target_descriptions =
		m_color_target_descriptions.data();
	graphics_pipeline_create_info.target_info.num_color_targets =
		static_cast<Uint32>(m_color_target_descriptions.size());
	graphics_pipeline_create_info.target_info.depth_stencil_format = m_depth_stencil_format;
	graphics_pipeline_create_info.target_info.has_depth_stencil_target = m_has_depth_stencil_target;

	UniqueGPUGraphicsPipeline pipeline =
		make_gpu_graphics_pipeline(m_device, graphics_pipeline_create_info);
	if (!pipeline) return fail_sdl("SDL_CreateGPUGraphicsPipeline");

	return pipeline;
}