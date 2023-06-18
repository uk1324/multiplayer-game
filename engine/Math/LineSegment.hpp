#pragma once

#include "Line.hpp"
#include "Aabb.hpp"
#include <array>

std::optional<Vec2> intersectLineSegments(Vec2 s0, Vec2 e0, Vec2 s1, Vec2 e1);

struct LineSegment {
	LineSegment(Line line, float minDistanceAlongLine, float maxDistanceAlongLine);
	LineSegment(Vec2 start, Vec2 end);

	auto closestPointTo(Vec2 p) const -> Vec2;
	auto distance(Vec2 p) const -> float;
	auto asBoxContains(float width, Vec2 p) const -> bool;
	auto asCapsuleContains(float thickness, Vec2 p) const -> bool;
	auto aabb() const -> Aabb;
	auto getCorners() const -> std::array<Vec2, 2>;
	// This is essentially just LineSegment vs LineSegment intersection
	auto raycastHit(Vec2 rayBegin, Vec2 rayEnd) const -> std::optional<Vec2>;
	auto intersection(const LineSegment& other) const -> std::optional<Vec2>;

	Line line;
	float minDistanceAlongLine;
	float maxDistanceAlongLine;
};