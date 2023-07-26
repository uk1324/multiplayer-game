#include "Random.hpp"
#include "Utils.hpp"
#include <random>

float random01() {
	std::random_device rd;
	std::mt19937 e(rd());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	return dist(e);
}

Vec2 randomPointInUnitCircle() {
	const auto r = sqrt(random01());
	const auto angle = random01() * TAU<float>;
	return Vec2::oriented(angle) * r;
}
