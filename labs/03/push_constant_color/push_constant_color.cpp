/**
 * @file push_constant_color.cpp
 * @author FaunJyng
 * @brief Pass color data from C++ to the Fragment Shader
 * using push constants. Render three triangles with different base colors.
 * @version 2.0
 * @date 19/11/2026
 */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <fmt/format.h>

#include "gpu_buffer_uploader.h"
#include "gpu_graphics_pipeline_creator.h"
#include "gpu_resource.h"
#include "gpu_shader_creator.h"
#include "path.h"
#include "window_claim_manager.h"

namespace {
	struct Vertex {
		float x, y;
	};

	struct Color {
		float r, g, b, a;
	};

	constexpr Vertex VERTICES[] = {
		{-0.9f, -0.2f},
		{-0.5f, 0.6f},
		{-0.1f, -0.2f},

		{-0.3f, -0.8f},
		{0.0f, -0.1f},
		{0.3f, -0.8f},

		{0.1f, -0.2f},
		{0.5f, 0.6f},
		{0.9f, -0.2f},
	};

	constexpr Color COLORS[] = {
		{1.0f, 0.2f, 0.2f, 1.0f},  // red
		{0.2f, 1.0f, 0.3f, 1.0f},  // green
		{0.2f, 0.4f, 1.0f, 1.0f},  // blue
	};

	struct AppState {
		UniqueSDLWindow window{nullptr};
		UniqueSDLGPUDevice device{nullptr};
		// Destroyed after GPU resources, but before device.
		WindowClaimManger window_claim_manager;
		UniqueGPUGraphicsPipeline pipeline;
		UniqueGPUBuffer vertex_buffer;
	};
}  // namespace

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	(void)argc;
	(void)argv;

	SDL_SetAppMetadata("Push Constant Color", "1.0", "com.faunjyng.sdl3labs");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[ERROR] SDL_Init() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto* state{new AppState{}};
	*appstate = state;

	UniqueSDLWindow window{SDL_CreateWindow("Push Constant Color", 800, 600, 0)};
	if (!window) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"[ERROR] SDL_CreateWindow() failed: %s",
			SDL_GetError()
		);
		return SDL_APP_FAILURE;
	}

	UniqueSDLGPUDevice device{SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr)};
	if (!device) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"[ERROR] SDL_CreateGPUDevice() failed: %s",
			SDL_GetError()
		);
		return SDL_APP_FAILURE;
	}

	state->window = std::move(window);
	state->device = std::move(device);

	if (!SDL_ClaimWindowForGPUDevice(state->device.get(), state->window.get())) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"[ERROR] SDL_ClaimWindowForGPUDevice() failed: %s",
			SDL_GetError()
		);
		return SDL_APP_FAILURE;
	}

	state->window_claim_manager.set_device(state->device.get());
	state->window_claim_manager.set_window(state->window.get());

	// Shaders
	auto vertex_shader_path{Path::absolute_path_relative_to_exe(
		"02",
		"push_constant_color",
		"shaders",
		"triangle.vert.spv"
	)};
	if (!vertex_shader_path) return SDL_APP_FAILURE;

	auto fragment_shader_path{Path::absolute_path_relative_to_exe(
		"02",
		"push_constant_color",
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
			.create(),
	};
	if (!vertex_shader) return SDL_APP_FAILURE;

	UniqueGPUShader fragment_shader{
		shader_creator.set_path(fragment_shader_path->c_str())
			.set_stage(SDL_GPU_SHADERSTAGE_FRAGMENT)
			.set_num_uniform_buffers(1)
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
			.add_vertex_buffer(
				{.slot = 0, .pitch = sizeof(Vertex), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX}
			)
			.add_vertex_attribute(
				{.location = 0,
				 .buffer_slot = 0,
				 .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				 .offset = 0}
			)
			.add_color_target(
				{.format =
					 SDL_GetGPUSwapchainTextureFormat(state->device.get(), state->window.get())}
			)
			.create()
	};
	if (!pipeline) return SDL_APP_FAILURE;
	state->pipeline = std::move(pipeline);

	// Vertex Buffer
	SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
	transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transfer_buffer_create_info.size = sizeof(VERTICES);
	UniqueGPUTransferBuffer transfer_buffer{
		SDL_CreateGPUTransferBuffer(state->device.get(), &transfer_buffer_create_info),
		{state->device.get()}
	};

	GPUBufferUploader buffer_uploader{};
	UniqueGPUBuffer vertex_buffer{
		buffer_uploader
			.set_error_handler([](std::string_view message) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", std::string(message).c_str());
			})
			.set_device(state->device.get())
			.set_data(VERTICES)
			.set_usage(SDL_GPU_BUFFERUSAGE_VERTEX)
			.set_transfer_buffer(transfer_buffer.get())
			.create()
	};
	if (!vertex_buffer) return SDL_APP_FAILURE;
	state->vertex_buffer = std::move(vertex_buffer);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	auto* state{static_cast<AppState*>(appstate)};

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
		SDL_GPUColorTargetInfo target{};
		target.texture = swapchain_texture;
		target.clear_color = {1.0f, 1.0f, 1.0f, 1.0f};
		target.load_op = SDL_GPU_LOADOP_CLEAR;
		target.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* render_pass{SDL_BeginGPURenderPass(command_buffer, &target, 1, nullptr)};
		if (!render_pass) {
			SDL_LogError(
				SDL_LOG_CATEGORY_APPLICATION,
				"[ERROR] SDL_BeginGPURenderPass() failed: %s",
				SDL_GetError()
			);
			SDL_CancelGPUCommandBuffer(command_buffer);
			return SDL_APP_FAILURE;
		}

		SDL_BindGPUGraphicsPipeline(render_pass, state->pipeline.get());

		SDL_GPUBufferBinding buffer_binding{state->vertex_buffer.get(), 0};
		SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);

		for (Uint32 i{0}; i < 3; ++i) {
			SDL_PushGPUFragmentUniformData(command_buffer, 0, &COLORS[i], sizeof(Color));
			SDL_DrawGPUPrimitives(render_pass, 3, 1, i * 3, 0);
		}

		SDL_EndGPURenderPass(render_pass);
	}

	if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
		SDL_LogError(
			SDL_LOG_CATEGORY_APPLICATION,
			"[ERROR] SDL_SubmitGPUCommandBuffer() failed: %s",
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
