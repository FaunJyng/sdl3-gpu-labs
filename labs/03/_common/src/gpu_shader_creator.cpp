#include "gpu_shader_creator.h"

#include <fmt/format.h>

#include <utility>

auto GPUShaderCreator::report(std::string_view message) const -> void
{
	if (m_error_handler) m_error_handler(message);
}

auto GPUShaderCreator::fail(std::string_view reason) const -> UniqueGPUShader
{
	report(fmt::format("[ERROR] GPUShaderCreator::create() failed: {}", reason));
	return {};
}

auto GPUShaderCreator::fail_sdl(std::string_view function) const -> UniqueGPUShader
{
	return fail(fmt::format("{}() failed: {}", function, SDL_GetError()));
}

auto GPUShaderCreator::set_error_handler(ErrorHandler handler) -> GPUShaderCreator&
{
	m_error_handler = std::move(handler);
	return *this;
}

auto GPUShaderCreator::set_device(SDL_GPUDevice* device) -> GPUShaderCreator&
{
	m_device = device;
	return *this;
}

auto GPUShaderCreator::set_path(std::string_view path) -> GPUShaderCreator&
{
	m_path = path;
	return *this;
}

auto GPUShaderCreator::set_format(SDL_GPUShaderFormat format) -> GPUShaderCreator&
{
	m_shader_create_info.format = format;
	return *this;
}

auto GPUShaderCreator::set_stage(SDL_GPUShaderStage stage) -> GPUShaderCreator&
{
	m_stage = stage;
	return *this;
}

auto GPUShaderCreator::set_entrypoint(std::string_view entrypoint) -> GPUShaderCreator&
{
	m_entrypoint = entrypoint;
	return *this;
}

auto GPUShaderCreator::set_num_samplers(Uint32 num_samplers) -> GPUShaderCreator&
{
	m_shader_create_info.num_samplers = num_samplers;
	return *this;
}

auto GPUShaderCreator::set_num_storage_buffers(Uint32 num_storage_buffers) -> GPUShaderCreator&
{
	m_shader_create_info.num_storage_buffers = num_storage_buffers;
	return *this;
}

auto GPUShaderCreator::set_num_storage_textures(Uint32 num_storage_textures) -> GPUShaderCreator&
{
	m_shader_create_info.num_storage_textures = num_storage_textures;
	return *this;
}

auto GPUShaderCreator::set_num_uniform_buffers(Uint32 num_uniform_buffers) -> GPUShaderCreator&
{
	m_shader_create_info.num_uniform_buffers = num_uniform_buffers;
	return *this;
}

auto GPUShaderCreator::set_props(SDL_PropertiesID props) -> GPUShaderCreator&
{
	m_shader_create_info.props = props;
	return *this;
}

auto GPUShaderCreator::create() const -> UniqueGPUShader
{
	if (!m_device) return fail("Device has not been set.");
	if (m_path.empty()) return fail("Shader path has not been set.");
	if (m_entrypoint.empty()) return fail("Entrypoint is empty.");
	if (m_shader_create_info.format == SDL_GPU_SHADERFORMAT_INVALID)
		return fail("Shader format has not been set.");
	if (!m_stage) return fail("shader stage has not been set.");
	if ((SDL_GetGPUShaderFormats(m_device) & m_shader_create_info.format) == 0) {
		return fail(fmt::format(
			"the device does not support the requested shader format "
			"(file: \"{}\").",
			m_path
		));
	}

	std::size_t code_size{0};
	UniqueSDLMemory code{SDL_LoadFile(m_path.c_str(), &code_size)};
	if (!code) {
		return fail(fmt::format("SDL_LoadFile(\"{}\") failed: {}", m_path, SDL_GetError()));
	}
	if (code_size == 0) {
		return fail(fmt::format("Shader file \"{}\" is empty.", m_path));
	}

	SDL_GPUShaderCreateInfo info{m_shader_create_info};
	info.code = static_cast<const Uint8*>(code.get());
	info.code_size = code_size;
	info.entrypoint = m_entrypoint.c_str();
	info.stage = *m_stage;

	UniqueGPUShader shader{SDL_CreateGPUShader(m_device, &info), {m_device}};
	if (!shader) {
		return fail(
			fmt::format("SDL_CreateGPUShader() failed for \"{}\": {}", m_path, SDL_GetError())
		);
	}

	return shader;
}