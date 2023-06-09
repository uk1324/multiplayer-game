#pragma once

#pragma once

#include <cmath>

template<typename T>
struct Vec4T {
	constexpr Vec4T();
	explicit constexpr Vec4T(const T& v);
	constexpr Vec4T(const T& x, const T& y, const T& z, const T& w = 1);

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