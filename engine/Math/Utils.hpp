#pragma once

#include "Vec2.hpp"

template<typename T>
constexpr T PI = static_cast<T>(3.14159265359);
template<typename T>
constexpr T TAU = PI<T> * static_cast<T>(2.0);
template<typename T>
constexpr T E = static_cast<T>(2.71828182846);

static constexpr float SIGNS[]{ -1.0f, 1.0f };

template<typename T, typename U>
auto lerp(T a, T b, U t) -> T {
	// This is better than a + (b - a) * t because it isn't as affected by rounding errors as much.
	return a * (1 - t) + b * t;
}

template<typename T, typename U>
auto cubicHermite(T a, T va, T b, T vb, U t) -> T {
	const auto t2 = t * t;
	const auto t3 = t2 * t;
	return (2.0f * t3 - 3.0f * t2 + 1.0f) * a
		+ (t3 - 2.0f * t2 + t) * va
		+ (-2.0f * t3 + 3.0f * t2) * b
		+ (t3 - t2) * vb;
}

template<typename T>
T smoothstep(T x) {
	if (x > 1.0f) {
		x = 1.0f;
	} else if (x < 0.0f) {
		x = 0.0f;
	}
	return x * x * (3.0f - 2.0f * x);
}

struct PosAndZoom {
	Vec2 pos;
	float zoom;
};

auto lerpPosAndZoom(const PosAndZoom& current, const PosAndZoom& target, float t) -> PosAndZoom;

template<typename T>
auto sign(T x) -> T {
	return x < 0 ? static_cast<T>(-1) : static_cast<T>(1);
}

template<typename T>
auto signOrZero(T x) -> T {
	if (x == 0) {
		return 0;
	} else if (x < 0) {
		return -1;
	} else {
		return 1;
	}
}

template<typename T>
auto degToRad(T s) -> T {
	return s * (TAU<T> / 360);
}

template<typename T>
auto radToDeg(T s) -> T {
	return s * (360 / TAU<T>);
}

// To move 2 conntected lines with unconstrained roatation to a point you just need to construct a triangle with the side lengths(using law of sines or cosines). There are 2 ways to do this. Use the triangle sides sum law to check if this is possible or check if it lies in a hollowed out circle. 
// If there are 3 lines there are infinite solutions for example piston rod engines.