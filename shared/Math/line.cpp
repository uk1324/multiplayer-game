#include <math/line.hpp>
#include <math/mat2.hpp>
#include <cmath>

Line::Line(Vec2 n, float d)
	: n{ n }
	, d{ d } {}

Line::Line(Vec2 a, Vec2 b) {
	n = (b - a).rotBy90deg().normalized();
	d = dot(n, b);
}

auto Line::translated(Vec2 v) const -> Line {
	return Line{ n, d + dot(n, v) };
}

auto Line::distanceAlong(Vec2 v) const -> float {
	return det(n, v);
}

auto Line::projectPointOntoLine(Vec2 p) const -> Vec2 {
	return p - n * distance(*this, p);
}

auto Line::intersection(const Line& other) const -> std::optional<Vec2> {
	// xA = b
	// x = inv(A) * b
	const Mat2 A{ n, other.n };
	if (A.det() == 0.0f) {
		return std::nullopt;
	}

	const Vec2 b{ d, other.d };
	return b * A.inversed();
}

auto signedDistance(const Line& l, Vec2 p) -> float {
	return l.d - dot(l.n, p);
}

auto distance(const Line& l, Vec2 p) -> float {
	return abs(signedDistance(l, p));
}
