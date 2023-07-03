#include <client/Debug.hpp>
#include <engine/Input/Input.hpp>

static float elapsed = 0.0f;
static float dt = 0.0f;

#include <iostream>

void Debug::update(float dt) {
	Debug::circles.clear();
	elapsed += dt;
	::dt = dt;
}

void Debug::drawCircle(Vec2 pos, float radius, Vec3 color) {
	circles.push_back({ pos, radius, color });
}

void Debug::scrollInput(float& value, float scalePerSecond) {
	if (Input::scrollDelta() > 0.0f)
		value *= pow(scalePerSecond, dt);
	if (Input::scrollDelta() < 0.0f)
		value /= pow(scalePerSecond, dt);
}

std::vector<Debug::Circle> Debug::circles;