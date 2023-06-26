#pragma once

#include "Iter2d.hpp"
#include <engine/Math/Vec4.hpp>
#include <optional>

struct Pixel32 {
	Pixel32(u8 r, u8 g, u8 b, u8 a = 255);
	explicit Pixel32(u8 v, u8 a = 255);
	Pixel32(const Vec4& color);

	u8 r, g, b, a;
};

struct Image32 {
	using IndexedPixelIterator = Iter2d<Pixel32>;
	explicit Image32(const char* path);
	explicit Image32(i64 width, i64 height);
	explicit Image32(const Pixel32* data, i64 width, i64 height);
	Image32(const Image32& other);
	Image32(Image32&& other) noexcept;
	Image32& operator=(const Image32& other);
	Image32& operator=(Image32&& other) noexcept;
	~Image32();

	std::optional<Image32> fromFile(const char* path);
	void saveToPng(const char* path) const;
	void copyAndResize(const Image32& other);

	Pixel32& operator()(i64 x, i64 y);
	const Pixel32& operator()(i64 x, i64 y) const;
	Vec2T<i64> size() const;
	i64 width() const;
	i64 height() const;
	Pixel32* data();
	const Pixel32* data() const;
	usize dataSizeBytes() const;
	usize pixelCount() const;

	Pixel32* begin();
	Pixel32* end();
	const Pixel32* cbegin() const;
	const Pixel32* cend() const;

	struct IndexedPixelRange {
		Image32& image;
		IndexedPixelIterator begin();
		IndexedPixelIterator end();
	};
	IndexedPixelRange indexed();
	template<typename T>
	void apply(T pixel32ToPixel32Function);
private:
	explicit Image32(const char* path, bool& loadedCorrectly);

protected:
	Pixel32* data_;
	Vec2T<i64> size_;
};