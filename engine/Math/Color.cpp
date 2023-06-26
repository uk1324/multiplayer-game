#include "Color.hpp"

Vec4 Color::scientificColoring(float v, float minV, float maxV) {
	v = std::min(std::max(v, minV), maxV - 0.01f);
	auto d = maxV - minV;
	v = d == 0.0f ? 0.5f : (v - minV) / d;
	auto m = 0.25f;
	int num = static_cast<int>(std::floor(v / m));
	auto s = (v - num * m) / m;

	float r = 0.0f, g = 0.0f, b = 0.0f;

	switch (num) {
	case 0: r = 0.0f; g = s; b = 1.0f; break;
	case 1: r = 0.0f; g = 1.0f; b = 1.0f - s; break;
	case 2: r = s; g = 1.0f; b = 0.0f; break;
	case 3: r = 1.0f; g = 1.0f - s; b = 0.0f; break;
	}

	if (r == 0.0f && g == 0.0f && b == 0.0f) {
		g = 1.0f;
	}

	return Vec4(r, g, b, 1.0f);
}

Vec4 Color::fromHsv(float h, float s, float v) {
	float hue = h * 360.f;

	float C = s * v;
	float X = C * (1.0f - std::abs(std::fmodf(hue / 60.0f, 2) - 1));
	float m = v - C;
	float r, g, b;
	if (hue >= 0 && hue < 60)
		r = C, g = X, b = 0;
	else if (hue >= 60 && hue < 120)
		r = X, g = C, b = 0;
	else if (hue >= 120 && hue < 180)
		r = 0, g = C, b = X;
	else if (hue >= 180 && hue < 240)
		r = 0, g = X, b = C;
	else if (hue >= 240 && hue < 300)
		r = X, g = 0, b = C;
	else
		r = C, g = 0, b = X;
	return Vec4(r + m, g + m, b + m, 1.0f);
}

Vec4 Color::toGrayscale(const Vec4& c) {
	auto v = c.x * 0.3f + c.y * 0.59f + c.z * 0.11f;
	return Vec4(v, v, v, c.w);
}
