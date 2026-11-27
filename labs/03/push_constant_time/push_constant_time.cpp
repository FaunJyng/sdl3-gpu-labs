/**
 * @file pushed_data_constant_position.cpp
 *DPushedData @author FaunJyng
 * @brief Pass time data from C++ to the Vertex Shader
 * using push constants to animate chaging color.
 * @version 2.0
 * @date 19/11/2026
 */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "gpu_graphics_pipeline_creator.h"
#include "gpu_resource.h"
#include "gpu_shader_creator.h"
#include "path.h"
#include "timer.h"
#include "window_claim_manager.h"

namespace {
	struct PushedData {
		float vertices[3][4];  // 3 vertices {x, y, unused, unused}
		float phase;
		float pad[3];  // 64 byte, match cbuffer
	};

	constexpr double SECONDS_PER_COLOR{1};
	constexpr double CYCLE_SECONDS{6.0 * SECONDS_PER_COLOR};

	struct AppState {
		UniqueSDLWindow window{nullptr};
		UniqueSDLGPUDevice device{nullptr};
		// Destroyed after GPU resources, but before device.
		WindowClaimManger window_claim_manager;
		UniqueGPUGraphicsPipeline pipeline;
		Timer timer;
	};
}  // namespace

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	(void)argc;
	(void)argv;

	SDL_SetAppMetadata("Push Constant Time", "1.0", "com.faunjyng.sdl3labs");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto* state{new AppState{}};
	*appstate = state;

	// Window + Device
	UniqueSDLWindow window{SDL_CreateWindow("Push Constant Time", 800, 600, 0)};
	if (!window) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	UniqueSDLGPUDevice device{SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr)};
	if (!device) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"SDL_CreateGPUDevice() failed: %s",
			SDL_GetError()
		);
		return SDL_APP_FAILURE;
	}

	state->window = std::move(window);
	state->device = std::move(device);

	// Claim window
	if (!SDL_ClaimWindowForGPUDevice(state->device.get(), state->window.get())) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"SDL_ClaimWindowForGPUDevice() failed: %s",
			SDL_GetError()
		);
		return SDL_APP_FAILURE;
	}

	state->window_claim_manager.set_device(state->device.get());
	state->window_claim_manager.set_window(state->window.get());

	// Shaders
	auto vertex_shader_path{Path::absolute_path_relative_to_exe(
		"03",
		"push_constant_time",
		"shaders",
		"triangle.vert.spv"
	)};
	if (!vertex_shader_path) return SDL_APP_FAILURE;

	auto fragment_shader_path{Path::absolute_path_relative_to_exe(
		"03",
		"push_constant_time",
		"shaders",
		"triangle.frag.spv"
	)};
	if (!fragment_shader_path) return SDL_APP_FAILURE;

	GPUShaderCreator shader_creator{};
	shader_creator.set_device(state->device.get())
		.set_error_handler([](std::string_view message) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", std::string(message).c_str());
		})
		.set_format(SDL_GPU_SHADERFORMAT_SPIRV);

	UniqueGPUShader vertex_shader{
		shader_creator.set_path(vertex_shader_path->c_str())
			.set_stage(SDL_GPU_SHADERSTAGE_VERTEX)
			.set_num_uniform_buffers(1)
			.create(),
	};
	if (!vertex_shader) return SDL_APP_FAILURE;

	UniqueGPUShader fragment_shader{
		shader_creator.set_path(fragment_shader_path->c_str())
			.set_stage(SDL_GPU_SHADERSTAGE_FRAGMENT)
			.set_num_uniform_buffers(0)
			.create(),
	};
	if (!fragment_shader) return SDL_APP_FAILURE;

	// Graphics Pipeline
	GPUGraphicsPipelineCreator graphics_pipeline_creator{};
	UniqueGPUGraphicsPipeline pipeline{
		graphics_pipeline_creator
			.set_error_handler([](std::string_view message) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", std::string(message).c_str());
			})
			.set_device(state->device.get())
			.set_vertex_shader(vertex_shader.get())
			.set_fragment_shader(fragment_shader.get())
			.set_primitive_type(SDL_GPU_PRIMITIVETYPE_TRIANGLELIST)
			.add_color_target(
				{.format =
					 SDL_GetGPUSwapchainTextureFormat(state->device.get(), state->window.get())}
			)
			.create()
	};
	if (!pipeline) return SDL_APP_FAILURE;
	state->pipeline = std::move(pipeline);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	auto* state{static_cast<AppState*>(appstate)};

	state->timer.Tick();

	const PushedData pushed_data{
		.vertices = {{-0.5f, -0.5f, 0, 0}, {0.0f, 0.5f, 0, 0}, {0.5f, -0.5f, 0, 0}},
		.phase = static_cast<float>(state->timer.ElapsedWrapped(CYCLE_SECONDS) / SECONDS_PER_COLOR),
		.pad = {}
	};

	SDL_GPUCommandBuffer* command_buffer{SDL_AcquireGPUCommandBuffer(state->device.get())};
	if (!command_buffer) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"SDL_AcquireGPUCommandBuffer() failed: %s",
			SDL_GetError()
		);
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchain_texture{nullptr};
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(
			command_buffer,
			state->window.get(),
			&swapchain_texture,
			nullptr,
			nullptr
		)) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"SDL_WaitAndAcquireGPUSwapchainTexture() failed: %s",
			SDL_GetError()
		);
		SDL_CancelGPUCommandBuffer(command_buffer);
		return SDL_APP_FAILURE;
	}

	if (swapchain_texture) {
		SDL_GPUColorTargetInfo color_target_info{
			.texture = swapchain_texture,
			.clear_color = {1.0f, 1.0f, 1.0f, 1.0f},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE
		};

		SDL_GPURenderPass* render_pass{
			SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr)
		};
		if (!render_pass) {
			SDL_LogError(
				SDL_LOG_CATEGORY_APPLICATION,
				"SDL_BeginGPURenderPass() failed: %s",
				SDL_GetError()
			);
			SDL_SubmitGPUCommandBuffer(command_buffer);
			return SDL_APP_FAILURE;
		}

		SDL_BindGPUGraphicsPipeline(render_pass, state->pipeline.get());

		SDL_PushGPUVertexUniformData(command_buffer, 0, &pushed_data, sizeof(PushedData));
		SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

		SDL_EndGPURenderPass(render_pass);
	}

	if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"SDL_SubmitGPUCommandBuffer() failed: %s",
			SDL_GetError()
		);
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	(void)appstate;

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	(void)result;

	delete static_cast<AppState*>(appstate);
}
