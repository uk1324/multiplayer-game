#pragma once

#include "Vec4.hpp"

namespace Color {
	static const Vec4 RED;
	static const Vec4 GREEN;
	static const Vec4 BLUE;
	static const Vec4 WHITE;
	static const Vec4 BLACK;

	Vec4 scientificColoring(float v, float minV, float maxV);
	Vec4 fromHsv(float h, float s, float v);
	Vec4 toGrayscale(const Vec4& c);
}