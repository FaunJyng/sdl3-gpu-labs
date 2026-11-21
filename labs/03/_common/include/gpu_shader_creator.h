#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "gpu_resource.h"

class GPUShaderCreator {
public:
	using ErrorHandler = std::function<void(std::string_view)>;

	auto set_error_handler(ErrorHandler handler) -> GPUShaderCreator&;
	auto set_device(SDL_GPUDevice* device) -> GPUShaderCreator&;
	auto set_path(std::string_view path) -> GPUShaderCreator&;
	auto set_format(SDL_GPUShaderFormat format) -> GPUShaderCreator&;
	auto set_stage(SDL_GPUShaderStage stage) -> GPUShaderCreator&;
	auto set_entrypoint(std::string_view entrypoint) -> GPUShaderCreator&;
	auto set_num_samplers(Uint32 num_samplers) -> GPUShaderCreator&;
	auto set_num_storage_buffers(Uint32 num_storage_buffers) -> GPUShaderCreator&;
	auto set_num_storage_textures(Uint32 num_storage_textures) -> GPUShaderCreator&;
	auto set_num_uniform_buffers(Uint32 num_uniform_buffers) -> GPUShaderCreator&;
	auto set_props(SDL_PropertiesID props) -> GPUShaderCreator&;

	[[nodiscard]] auto create() const -> UniqueGPUShader;

private:
	ErrorHandler m_error_handler{nullptr};

	SDL_GPUDevice* m_device{nullptr};
	// Used only for the num fields, `format`, and `props`. `code`, `code_size`, `entrypoint`, and
	// `stage` are populated in `create()`.
	SDL_GPUShaderCreateInfo m_shader_create_info{};
	std::optional<SDL_GPUShaderStage> m_stage;
	std::string m_path;
	std::string m_entrypoint{"main"};

	auto report(std::string_view message) const -> void;
	auto fail(std::string_view reason) const -> UniqueGPUShader;
	auto fail_sdl(std::string_view function) const -> UniqueGPUShader;
};