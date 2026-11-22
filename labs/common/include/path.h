#pragma once
#include <SDL3/SDL.h>

#include <filesystem>
#include <optional>
#include <utility>

class Path {
public:
	template <typename... Args>
	static auto absolute_path_relative_to_exe(Args&&... paths) -> std::optional<std::string>
	{
		const char* base_path{SDL_GetBasePath()};
		if (!base_path) {
			SDL_LogError(
				SDL_LOG_CATEGORY_APPLICATION,
				"[ERROR] Path::absolute_path_relative_to_exe() failed: "
				"SDL_GetBasePath() failed: %s",
				SDL_GetError()
			);
			return std::nullopt;
		}
		std::filesystem::path full_path{base_path};
		(full_path /= ... /= std::forward<Args>(paths));
		return full_path.string();
	}
};