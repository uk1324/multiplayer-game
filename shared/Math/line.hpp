#pragma once

#include <math/vec2.hpp>

#include <optional>

struct Line {
	Line(Vec2 n, float d);
	// The normal is the line from a to b rotated counterclockwise.
	Line(Vec2 a, Vec2 b);
	auto translated(Vec2 v) const -> Line;
	auto distanceAlong(Vec2 v) const -> float;
	auto projectPointOntoLine(Vec2 p) const -> Vec2;
	// Returns nullopt if there are no solutions, or infinitely many solutions. Lines are parallel.
	auto intersection(const Line& other) const -> std::optional<Vec2>;

	// dot(n, p) = d
	// This means that the line center is located at the vector n * d and the positive half space lies in the direction of the normal.

	// Unit length.
	Vec2 n;
	float d;
};

auto signedDistance(const Line& l, Vec2 p) -> float;
auto distance(const Line& l, Vec2 p) -> float;