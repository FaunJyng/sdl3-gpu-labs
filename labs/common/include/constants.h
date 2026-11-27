#pragma once

#include <numbers>

namespace Number {
	inline constexpr double PI = std::numbers::pi;
	inline constexpr double TAU = std::numbers::pi_v<double> * 2.0;
	inline constexpr double SQRT2 = std::numbers::sqrt2;
	inline constexpr double E = std::numbers::e;
}  // namespace Number