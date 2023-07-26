#pragma once

#include <Assertions.hpp>
#include <Types.hpp>

#include <algorithm>
#include <ostream>
#include <cmath>

// TODO: Methods // points[i].distance_to(points[i - 1]) and angle_to

template<typename T>
struct Vec2T {
	constexpr Vec2T();
	constexpr explicit Vec2T(T all);
	constexpr Vec2T(T x, T y);
	template<typename U> 
	explicit constexpr Vec2T(Vec2T<U> other);
	static auto oriented(T angle) -> Vec2T;
	static auto fromPolar(T angle, T length) -> Vec2T;

	auto clamped(T min, T max) const -> Vec2T;
	auto clamped(Vec2T min, Vec2T max) const -> Vec2T;
	auto lengthSq() const -> T;
	auto length() const -> T;
	auto angle() const -> float;
	auto normalized() const -> Vec2T;
	auto rotBy90deg() const -> Vec2T;
	auto applied(T (*function)(T)) const -> Vec2T;
	auto min(const Vec2T& other) const -> Vec2T;
	auto max(const Vec2T& other) const -> Vec2T;
	auto scaledAround(const Vec2T& point, const Vec2T& scale) const -> Vec2T;
	auto xOverY() const -> float; // This should return a float not T.
	auto flippedY() const -> Vec2T;
	T distanceTo(Vec2T p) const;
	T distanceSquaredTo(Vec2T p) const;
	Vec2T roundedToDecimalDigits(int digitsCount) const;

	auto operator+(const Vec2T& v) const -> Vec2T;
	auto operator+=(const Vec2T& v) -> Vec2T&;
	auto operator-(const Vec2T& v) const -> Vec2T;
	auto operator-=(const Vec2T& v) -> Vec2T&;
	auto operator*(T s) const -> Vec2T;
	auto operator*=(T s) -> Vec2T&;
	auto operator*(const Vec2T& v) const -> Vec2T;
	auto operator*=(const Vec2T& v) -> Vec2T&;
	auto operator/(T s) const -> Vec2T;
	auto operator/=(T s) -> Vec2T;
	auto operator/(const Vec2T& v) const -> Vec2T;
	auto operator/=(const Vec2T& v) -> Vec2T&;
	auto operator-() const -> Vec2T;

	auto operator[](isize index) -> T&;
	auto operator[](isize index) const -> const T&;

	// Not using <=> because it doesn't make sense for the other operators.
	auto operator==(const Vec2T& other) const -> bool;
	auto operator!=(const Vec2T& other) const -> bool;

	auto data() -> T*;
	auto data() const -> const T*;

	T x, y;
};

using Vec2 = Vec2T<float>;

template<typename T> 
auto operator<< (std::ostream& os, const Vec2T<T>& v) -> std::ostream& {
	os << '[' << v.x << ", " << v.y << ']';
	return os;
}

template<typename T>
auto operator*(T s, const Vec2T<T>& v) -> Vec2T<T> {
	return v * s;
}

template<typename T>
auto operator/(T s, const Vec2T<T>& v) -> Vec2T<T> {
	return Vec2{ s / v.x, s / v.y };
}

template<typename T>
auto dot(const Vec2T<T>& a, const Vec2T<T> b) -> T {
	return a.x * b.x + a.y * b.y;
}

template<typename T>
auto det(const Vec2T<T>& a, const Vec2T<T> b) -> T {
	return a.x * b.y - b.x * a.y;
}

template<typename T>
auto distance(const Vec2T<T>& a, const Vec2T<T> b) -> T {
	return (a - b).length();
}

template<typename T>
auto cross(const Vec2T<T>& a, const Vec2T<T>& b) -> T {
	return a.x * b.y - a.y * b.x;
}

template<typename T>
auto cross(const Vec2T<T>& a, T s) -> Vec2T<T> {
	return Vec2T{ s * a.y, -s * a.x };
}

template<typename T>
auto cross(T s, const Vec2T<T>& a) -> Vec2 {
	return Vec2T{ -s * a.y, s * a.x };
}

// I don't think there are any good ways to leaves values unitialized explicitly so I just have to use the default constrcutor. Leaving the values unitialized is useful when creating an array. This could also be solved by providing some intialization function or in some cases maybe using a statically sized vector.
template<typename T>
constexpr Vec2T<T>::Vec2T()
	: x(T(123123123))
	, y(T(123123123)) {}

template<typename T>
constexpr Vec2T<T>::Vec2T(T all)
	: x(all)
	, y(all) {}

template<typename T>
constexpr Vec2T<T>::Vec2T(T x, T y)
	: x(x)
	, y(y) {}

template<typename T>
auto Vec2T<T>::oriented(T angle) -> Vec2T {
	return Vec2{ cos(angle), sin(angle) };
}

template<typename T>
auto Vec2T<T>::fromPolar(T angle, T length) -> Vec2T {
	return oriented(angle) * length;
}

template<typename T>
auto Vec2T<T>::clamped(T min, T max) const -> Vec2T {
	return Vec2T{ std::clamp(x, min, max), std::clamp(y, min, max) };
}

template<typename T>
auto Vec2T<T>::clamped(Vec2T min, Vec2T max) const -> Vec2T {
	return Vec2T{ std::clamp(x, min.x, max.x), std::clamp(y, min.y, max.y) };
}

template<typename T>
auto Vec2T<T>::lengthSq() const -> T {
	return x * x + y * y;
}

template<typename T>
auto Vec2T<T>::length() const -> T {
	return sqrtf(lengthSq());
}

template<typename T>
auto Vec2T<T>::angle() const -> float {
	return atan2(y, x);
}

template<typename T>
auto Vec2T<T>::normalized() const -> Vec2T {
	const auto l = length();
	if (l == 0) {
		return *this;
	}
	return *this / l;
}

template<typename T>
auto Vec2T<T>::rotBy90deg() const -> Vec2T {
	return Vec2{ y, -x };
}

template<typename T>
auto Vec2T<T>::applied(T (*function)(T)) const -> Vec2T {
	return Vec2{ function(x), function(y) };
}

template<typename T>
auto Vec2T<T>::min(const Vec2T& other) const -> Vec2T {
	return Vec2T{ std::min(x, other.x), std::min(y, other.y) };
}

template<typename T>
auto Vec2T<T>::max(const Vec2T& other) const -> Vec2T {
	return Vec2T{ std::max(x, other.x), std::max(y, other.y) };
}

template<typename T>
auto Vec2T<T>::scaledAround(const Vec2T& point, const Vec2T& scale) const -> Vec2T {
	auto result = *this;
	result -= point;
	result *= scale;
	result += point;
	return result;
}

template<typename T>
auto Vec2T<T>::xOverY() const -> float {
	return static_cast<float>(x) / static_cast<float>(y);
}

template<typename T>
auto Vec2T<T>::flippedY() const -> Vec2T {
	return Vec2T{ x, -y };
}

template<typename T>
T Vec2T<T>::distanceTo(Vec2T p) const {
	return distance(*this, p);
}

template<typename T>
T Vec2T<T>::distanceSquaredTo(Vec2T p) const {
	(*this - p).lengthSquared();
}

template<typename T>
Vec2T<T> Vec2T<T>::roundedToDecimalDigits(int digitsCount) const {
	const auto scale = pow(10.0f, digitsCount);
	return (*this * scale).applied(floor) / scale;
}

template<typename T>
auto Vec2T<T>::operator+(const Vec2T& v) const -> Vec2T {
	return Vec2T{ x + v.x, y + v.y };
}

template<typename T>
auto Vec2T<T>::operator+=(const Vec2T& v) -> Vec2T& {
	*this = *this + v;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator-(const Vec2T& v) const -> Vec2T {
	return Vec2T{ x - v.x, y - v.y };
}

template<typename T>
auto Vec2T<T>::operator-=(const Vec2T& v) -> Vec2T& {
	*this = *this - v;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator*(T s) const -> Vec2T {
	return Vec2T{ x * s, y * s };
}

template<typename T>
auto Vec2T<T>::operator*=(T s) -> Vec2T& {
	*this = *this * s;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator*(const Vec2T& v) const -> Vec2T {
	return Vec2T{ x * v.x, y * v.y };
}

template<typename T>
auto Vec2T<T>::operator*=(const Vec2T& v) -> Vec2T& {
	*this = *this * v;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator/(T s) const -> Vec2T {
	return Vec2T{ x / s, y / s };
}

template<typename T>
auto Vec2T<T>::operator/=(T s) -> Vec2T {
	*this = *this / s;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator/(const Vec2T& v) const -> Vec2T {
	return Vec2T{ x / v.x, y / v.y };
}

template<typename T>
auto Vec2T<T>::operator/=(const Vec2T& v) -> Vec2T& {
	*this = *this / v;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator-() const -> Vec2T {
	return Vec2T{ -x, -y };
}

template<typename T>
auto Vec2T<T>::operator[](isize index) -> T& {
	return const_cast<T&>((const_cast<const Vec2&>(*this))[index]);
}

template<typename T>
auto Vec2T<T>::operator[](isize index) const -> const T& {
	switch (index) {
	case 0: return x;
	case 1: return y;
	default:
		ASSERT_NOT_REACHED();
		return *reinterpret_cast<const T*>(nullptr);
	}
}

template<typename T>
auto Vec2T<T>::operator==(const Vec2T& other) const -> bool {
	return (x == other.x) && (y == other.y);
}

template<typename T>
auto Vec2T<T>::operator!=(const Vec2T& other) const -> bool {
	return !(*this == other);
}

template<typename T>
auto Vec2T<T>::data() -> T* {
	return &x;
}

template<typename T>
auto Vec2T<T>::data() const -> const T* {
	return &x;
}

template<typename T>
template<typename U>
constexpr Vec2T<T>::Vec2T(Vec2T<U> other)
	: x{ static_cast<T>(other.x) }
	, y{ static_cast<T>(other.y) } {}
