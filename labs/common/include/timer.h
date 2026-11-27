#pragma once
#include <SDL3/SDL.h>

#include <cmath>

struct Timer {
	void Tick();
	auto DeltaTime() const -> float { return static_cast<float>(delta); }
	auto Elapsed() const -> float { return static_cast<float>(elapsed); }
	auto ElapsedWrapped(double period) const -> float
	{
		return static_cast<float>(std::fmod(elapsed, period));
	}

private:
	Uint64 saved_counter{SDL_GetPerformanceCounter()};
	double delta{0.0};
	double elapsed{0.0};
};