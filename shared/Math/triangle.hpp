#pragma once

#include <math/vec2.hpp>

struct Triangle {
	Triangle(Vec2 v0, Vec2 v1, Vec2 v2);
	Vec2 v[3];
	/*auto counterClockwiseContains(Vec2 p) const -> bool;
	auto clockwiseContains(Vec2 p) const -> bool;*/
	auto contains(Vec2 p) const -> bool;
	auto containsWithEpsilon(Vec2 p, float epsilon) -> bool;
	auto isClockwise() const -> bool;
	auto area() -> bool;
};