#include "Color32.hpp"

Color32::Color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	: r(r)
	, g(g)
	, b(b)
	, a(a)
{}

Color32 Color32::operator+(Color32 rhs) const
{
	return Color32(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
}

Color32 Color32::operator*(float rhs) const
{
	return Color32(r * rhs, g * rhs, b * rhs, a * rhs);
}
