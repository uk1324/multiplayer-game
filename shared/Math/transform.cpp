#include <math/transform.hpp>

Rotation::Rotation(float cos, float sin)
	: sin{ sin }
	, cos{ cos } {}

Rotation::Rotation(float angle)
	: sin{ ::sin(angle) }
	, cos{ ::cos(angle) } {}

Transform::Transform(Vec2 pos, Rotation rot)
	: pos{ pos }
	, rot{ rot } {}

Transform::Transform(Vec2 pos, float angle)
	: pos{ pos }
	, rot{ angle } {}

auto Transform::operator*(const Transform& other) const -> Transform {
	// (v * A.r + A.p) * B.r + B.p
	// Distributivity of matrix multiplication over addition.
	// v * A.r * B.r + A.p * B.r + B.p
	// p = A.p * B.r + B.p
	// r = A.r * B.r
	return Transform{ pos * other.rot + other.pos, rot * other.rot };
}

auto Transform::angle() const -> float {
	return atan2(rot.sin, rot.cos);
}

auto Transform::inversed() const -> Transform {
	// v * r + p
	// (v - p) * r.inv()
	// v * r.inv() - p * r.inv()
	const auto rotInv = rot.inversed();
	return Transform{ -(pos * rotInv), rotInv };
}

const Transform Transform::identity = Transform{ Vec2{ 0.0f }, 0.0f };

auto Rotation::operator*(const Rotation& other) const -> Rotation {
	// Complex multiplication.
	return Rotation{ cos * other.cos - sin * other.sin, cos * other.sin + other.cos * sin };
}

auto Rotation::operator*=(const Rotation& other) -> Rotation& {
	*this = *this * other;
	return *this;
}

auto Rotation::inversed() const -> Rotation {
	return Rotation{ cos, -sin };
}

auto Rotation::toMatrix() const -> Mat2 {
	return Mat2{ Vec2{ cos, sin }, Vec2{ -sin, cos } };
}

auto operator*(const Vec2& v, const Rotation& rot) -> Vec2 {
	return v.x * Vec2{ rot.cos, rot.sin } + v.y * Vec2{ -rot.sin, rot.cos };
}

auto operator*=(Vec2& v, const Rotation& rot) -> Vec2& {
	v = v * rot;
	return v;
}

auto operator*(const Vec2& v, const Transform& transform) -> Vec2 {
	return v * transform.rot + transform.pos;
}

auto operator*=(Vec2& v, const Transform& transform) -> Vec2& {
	v = v * transform;
	return v;
}
