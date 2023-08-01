#pragma once

#include <engine/Math/Vec2.hpp>

template<typename T>
struct Iter2d {
	Vec2T<i64> pos;
	T* data;
	i64 rowWidth;

	operator T& ();
	auto operator=(const T& value)->T&;
	auto operator++()->Iter2d&;
	auto operator*()->Iter2d&;
	auto operator!=(const Iter2d& other) -> bool;
};

template<typename T>
Iter2d<T>::operator T& () {
	return *data;
}

template<typename T>
auto Iter2d<T>::operator=(const T& value) -> T& {
	*data = value;
	return *data;
}

template<typename T>
auto Iter2d<T>::operator++() -> Iter2d& {
	data++;
	pos.x++;
	if (pos.x >= rowWidth) {
		pos.x = 0;
		pos.y++;
	}
	return *this;
}

template<typename T>
auto Iter2d<T>::operator*() -> Iter2d& {
	return *this;
}

template<typename T>
auto Iter2d<T>::operator!=(const Iter2d& other) -> bool {
	return data != other.data;
}