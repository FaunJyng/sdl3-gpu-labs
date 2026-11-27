#include "timer.h"

void Timer::Tick()
{
	static const double frequency{static_cast<double>(SDL_GetPerformanceFrequency())};

	const Uint64 now{SDL_GetPerformanceCounter()};
	delta = static_cast<double>(now - saved_counter) / frequency;
	saved_counter = now;
	elapsed += delta;
}