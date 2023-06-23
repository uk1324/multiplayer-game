#pragma once

#include <client/PtVertex.hpp>
#include <engine/Math/Vec3.hpp>
#include <span>
#include <vector>

float triangulateLine(std::span<const Vec2> line, float width, std::vector<PtVertex>& output);