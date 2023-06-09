#include <utils/timer.hpp>

Timer::Timer()
	: start{ std::chrono::high_resolution_clock::now() } {}

auto Timer::elapsedMilliseconds() const -> float {
	std::chrono::duration<float, std::milli> elapsed = (std::chrono::high_resolution_clock::now() - start);
	return elapsed.count();
}