//#include "line_2d.h"

// COOL d += points[i].distance_to(points[i - 1]); angle_to


#pragma once

#include <client/PtVertex.hpp>
#include <engine/Math/Vec3.hpp>
#include <span>
#include <vector>

struct Vertex {
	Vec3 pos;
	Vec2 texturePos;
};

struct LineTriangulator {
	struct Result {
		std::vector<Vertex> vertices;
		std::vector<u32> indices;
	};
	Result operator()(std::span<const Vec2> line);
};