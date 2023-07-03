#pragma once

#include <cmath>
#include "Vec3.hpp"

template<typename T>
struct Vec4T {
	constexpr Vec4T();
	explicit constexpr Vec4T(const T& v);
	constexpr Vec4T(const T& x, const T& y, const T& z, const T& w = 1);
	constexpr Vec4T(Vec3T<T> v, const T& w = 1);

	T* data();
	const T* data() const;

	T x, y, z, w;
};

using Vec4 = Vec4T<float>;

// TODO: Remove
template<typename T>
constexpr Vec4T<T>::Vec4T() 
	: x(123)
	, y(123)
	, z(123) 
	, w(123) {}

template<typename T>
constexpr Vec4T<T>::Vec4T(const T& v)
	: x(v)
	, y(v)
	, z(v)
	, w(1) {}

template<typename T>
constexpr Vec4T<T>::Vec4T(const T& x, const T& y, const T& z, const T& w)
	: x(x)
	, y(y)
	, z(z)
	, w(w) {}

template<typename T>
constexpr Vec4T<T>::Vec4T(Vec3T<T> v, const T& w) 
	: x(v.x)
	, y(v.y)
	, z(v.z)
	, w(w) {}

template<typename T>
inline T* Vec4T<T>::data() {
	return &x;
}

template<typename T>
inline const T* Vec4T<T>::data() const {
	return &x;
}
