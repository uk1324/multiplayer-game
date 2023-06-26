#pragma once

#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>
#include <vector>

namespace Debug {
	void drawCircle(Vec2 pos, float radius, Vec3 color = Vec3(1.0f));

	// TODO: dragablePoint() // id or source line.
	struct Circle {
		Vec2 pos;
		float radius;
		Vec3 color;
	};
	extern std::vector<Circle> circles;
}