#pragma once

#include <math/line.hpp>

#include <optional>

auto rayVsLine(Vec2 v, const Line& line) -> std::optional<float>;

//auto rayVsLine(Vec2 v, const Line& line) -> std::optional<float> {
//	/*
//	dot(tv) - d = 0
//	t * dot(v) = d
//	t = d / dot(v)
//	or
//	cos = dot(tv)
//	d = adjacentLength
//	hypotenuseLength = adjacentLength / cos
//	*/
//	float cos = dot(v, line.n);
//	// Parallel
//	if (cos == 0.0f)
//		return std::nullopt;
//
//	return line.d / cos;
//}