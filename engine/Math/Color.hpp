#pragma once

#include "Vec4.hpp"

namespace Color {
	Vec4 scientificColoring(float v, float minV, float maxV);
	Vec4 fromHsv(float h, float s, float v);
	Vec4 toGrayscale(const Vec4& c);
}

namespace Color3 {
	static constexpr Vec3 RED(1.0f, 0.0f, 0.0f);
	static constexpr Vec3 GREEN(0.0f, 1.0f, 0.0f);
	static constexpr Vec3 BLUE(0.0f, 0.0f, 1.0f);
	static constexpr Vec3 WHITE(1.0f);
	static constexpr Vec3 BLACK(0.0f);
}