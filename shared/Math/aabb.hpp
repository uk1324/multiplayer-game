#pragma once

#include <utils/span.hpp>
#include <shared/Math/vec2.hpp>

#include <array>

struct Aabb {
	Aabb(Vec2 min, Vec2 max);

	static auto fromPosSize(Vec2 pos, Vec2 size) -> Aabb;
	static auto fromCorners(Vec2 a, Vec2 b) -> Aabb;
	static auto fromPoints(Span<const Vec2> points) -> Aabb;

	auto size() const->Vec2;
	auto contains(Vec2 p) const -> bool;
	auto contains(const Aabb& aabb) const -> bool;
	auto combined(const Aabb& aabb) const -> Aabb;
	auto extended(Vec2 point) const -> Aabb;
	auto addedPadding(float padding) const -> Aabb;
	auto area() const -> float;
	auto collides(const Aabb& other) const -> bool;
	auto rayHits(Vec2 start, Vec2 end) const -> bool;
	auto center() const->Vec2;
	auto getCorners() const->std::array<Vec2, 4>;

	auto operator==(const Aabb& other) const -> bool = default;

	Vec2 min;
	Vec2 max;
};