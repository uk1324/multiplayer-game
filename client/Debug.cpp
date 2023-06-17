#include <client/Debug.hpp>

void Debug::drawCircle(Vec2 pos, float radius, Vec3 color) {
	circles.push_back({ pos, radius, color });
}

std::vector<Debug::Circle> Debug::circles;