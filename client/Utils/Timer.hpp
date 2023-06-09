#pragma once

#include <chrono>

struct Timer {
	Timer();
	auto elapsedMilliseconds() const -> float;
	std::chrono::high_resolution_clock::time_point start;
};