#pragma once

#include <iterator>
#include <vector>
#include <shared/Utils/Assertions.hpp>
#include <shared/Utils/Types.hpp>

template<typename T>
class Span {
public:
	Span(T* data, size_t size);
	Span(std::vector<T>& v);
	Span(const std::vector<std::remove_const_t<T>>& v);
	template<usize SIZE>
	Span(const T(&array)[SIZE]);

	auto operator[](usize index)->T&;
	auto operator[](usize index) const -> const T&;

	auto data() -> T*;
	auto data() const -> const T*;
	auto size() const->size_t;
	auto back() const -> const T&;

	auto begin() -> T*;
	auto end() -> T*;
	auto cbegin() const -> const T*;
	auto cend() const -> const T*;

	auto rbegin() -> std::reverse_iterator<T*>;
	auto rend() -> std::reverse_iterator<T*>;
	auto crbegin() const->std::reverse_iterator<const T*>;
	auto crend() const->std::reverse_iterator<const T*>;

private:
	T* data_;
	usize size_;
};

template<typename T>
Span<T>::Span(T* data, size_t size)
	: data_(data)
	, size_(size) {}

template<typename T>
Span<T>::Span(std::vector<T>& v)
	: data_{ v.data() }
	, size_{ v.size() } {}

template<typename T>
Span<T>::Span(const std::vector<std::remove_const_t<T>>& v)
	: data_{ v.data() }
	, size_{ v.size() } {}

template<typename T>
template<usize SIZE>
Span<T>::Span(const T(&array)[SIZE])
	: data_{ array }
	, size_{ SIZE } {}

template<typename T>
auto Span<T>::operator[](usize index) -> T& {
	ASSERT(index < size_);
	return data_[index];
}

template<typename T>
auto Span<T>::operator[](usize index) const -> const T& {
	return const_cast<T&>((const_cast<Span<T>&>(*this))[index]);
}

template<typename T>
auto Span<T>::data() -> T* {
	return data_;
}

template<typename T>
auto Span<T>::data() const -> const T* {
	return data_;
}

template<typename T>
auto Span<T>::size() const -> size_t {
	return size_;
}

template<typename T>
auto Span<T>::back() const -> const T& {
	return data_[size_ - 1];
}

template<typename T>
auto Span<T>::begin() -> T* {
	return data_;
}

template<typename T>
auto Span<T>::end() -> T* {
	return data_ + size_;
}

template<typename T>
auto Span<T>::cbegin() const -> const T* {
	return data_;
}

template<typename T>
auto Span<T>::cend() const -> const T* {
	return data_ + size_;
}

template<typename T>
auto Span<T>::rbegin() -> std::reverse_iterator<T*> {
	return std::reverse_iterator{ end() };
}

template<typename T>
auto Span<T>::rend() -> std::reverse_iterator<T*> {
	return std::reverse_iterator{ begin() };
}

template<typename T>
auto Span<T>::crbegin() const -> std::reverse_iterator<const T*> {
	return std::reverse_iterator{ cend() };
}

template<typename T>
auto Span<T>::crend() const -> std::reverse_iterator<const T*> {
	return std::reverse_iterator{ cbegin() };
}