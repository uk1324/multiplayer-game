#pragma once

#include <cmath>

template<typename T>
struct Vec3T {
	constexpr Vec3T(const T& v);
	constexpr Vec3T(const T& x, const T& y, const T& z);
	template<typename U>
	constexpr Vec3T(const Vec3T<U>& v);

	auto applied(T(*function)(T)) const -> Vec3T;
	auto operator*(const Vec3T& v) const -> Vec3T;
	auto operator*=(const Vec3T& v) -> Vec3T&;
	auto operator*(const T& s) const -> Vec3T;
	auto operator+(const Vec3T& v) const -> Vec3T;
	auto operator+=(const Vec3T& v) -> Vec3T&;
	auto operator-(const Vec3T& v) const -> Vec3T;
	constexpr auto operator/(const T& s) const -> Vec3T;
	auto operator/=(const T& s) -> Vec3T&;
	auto length() const -> float;
	auto normalized() const -> Vec3T;

	auto data() -> T*;
	auto data() const -> const T*;

	//// C++ doesn't allow constexpr static members of incomplete types because the initializer has to be inline.
	//static const Vec3T RED;
	//static const Vec3T GREEN;
	//static const Vec3T BLUE;
	//static const Vec3T WHITE;
	//static const Vec3T BLACK;

	T x, y, z;
};

using Vec3 = Vec3T<float>;

template<typename T>
constexpr Vec3T<T>::Vec3T(const T& v) 
	: x{ v }
	, y{ v } 
	, z{ v } {}

template<typename T>
constexpr Vec3T<T>::Vec3T(const T& x, const T& y, const T& z) 
	: x{ x } 
	, y{ y }
	, z{ z } {}

template<typename T>
auto Vec3T<T>::applied(T(*function)(T)) const -> Vec3T {
	return Vec3T{ function(x), function(y), function(z) };
}

template<typename T>
auto Vec3T<T>::operator*(const Vec3T& v) const -> Vec3T {
	return { x * v.x, y * v.y, z * v.z };
}

template<typename T>
auto Vec3T<T>::operator*=(const Vec3T& v) -> Vec3T& {
	*this = *this * v;
	return *this;
}

template<typename T>
auto Vec3T<T>::operator*(const T& s) const -> Vec3T {
	return Vec3T{ x * s, y * s, z * s };
}

template<typename T>
auto Vec3T<T>::operator+(const Vec3T& v) const -> Vec3T {
	return Vec3T{ x + v.x, y + v.y, z + v.z };
}

template<typename T>
auto Vec3T<T>::operator+=(const Vec3T& v) -> Vec3T& {
	*this = *this + v;
	return *this;
}

template<typename T>
auto Vec3T<T>::operator-(const Vec3T& v) const -> Vec3T {
	return Vec3T{ x - v.x, y - v.y, z - v.z };
}

template<typename T>
constexpr auto Vec3T<T>::operator/(const T& s) const -> Vec3T {
	return { x / s, y / s, z / s };
}

template<typename T>
auto Vec3T<T>::operator/=(const T& s) -> Vec3T& {
	*this = *this / s;
	return *this;
}

template<typename T>
auto Vec3T<T>::length() const -> float {
	return sqrt(x * x + y * y + z * z);
}

template<typename T>
auto Vec3T<T>::normalized() const -> Vec3T {
	const auto l = length();
	if (l == 0.0f) {
		return *this;
	}
	return *this / l;
}

template<typename T>
inline auto Vec3T<T>::data() -> T* {
	return &x;
}

template<typename T>
inline auto Vec3T<T>::data() const -> const T* {
	return &x;
}

//template<typename T> const Vec3T<T> Vec3T<T>::RED{ 1, 0, 0 };
//template<typename T> const Vec3T<T> Vec3T<T>::GREEN{ 0, 1, 0 };
//template<typename T> const Vec3T<T> Vec3T<T>::BLUE{ 0, 0, 1 };
//template<typename T> const Vec3T<T> Vec3T<T>::WHITE{ 1, 1, 1 };
//template<typename T> const Vec3T<T> Vec3T<T>::BLACK{ 0, 0, 0 };

template<typename T>
template<typename U>
constexpr Vec3T<T>::Vec3T(const Vec3T<U>& v)
	: x{ static_cast<T>(v.x) }
	, y{ static_cast<T>(v.y) }
	, z{ static_cast<T>(v.z) } {}
