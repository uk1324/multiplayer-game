#include "Image32.hpp"
#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>
#include <stb_image/stb_image_resize.h>
#include <string.h>

Image32::Image32(const char* path, bool& loadedCorrectly) {
	int x, y, channelCount;
	data_ = reinterpret_cast<Pixel32*>(stbi_load(path, &x, &y, &channelCount, STBI_rgb_alpha));
	loadedCorrectly = data_ != nullptr;
	size_ = { static_cast<usize>(x), static_cast<usize>(y) };
}

static thread_local bool loadedCorrectly;

Image32::Image32(const char* path)
	: Image32{ path, loadedCorrectly } {
	ASSERT(loadedCorrectly);
}

Image32::~Image32() {
	free(data_);
}

Image32::Image32(i64 width, i64 height)
	: size_(width, height)
	, data_(reinterpret_cast<Pixel32*>(malloc(4 * width * height))) {}

Image32::Image32(const Pixel32* data, i64 width, i64 height)
	: Image32(width, height) {
	memcpy(data_, data, dataSizeBytes());
}

Image32::Image32(const Image32& other)
	: size_{ other.size_ }
	, data_{ reinterpret_cast<Pixel32*>(malloc(other.dataSizeBytes())) } {
	if (data_ == nullptr) {
		ASSERT_NOT_REACHED();
		size_ = Vec2T<i64>{ 0 };
		return;
	}
	memcpy(data_, other.data_, other.dataSizeBytes());
}

Image32::Image32(Image32&& other) noexcept
	: size_(other.size_)
	, data_(other.data_) {
	other.data_ = nullptr;
}

Image32& Image32::operator=(const Image32& other) {
	if (other.dataSizeBytes() > dataSizeBytes()) {
		free(data_);
		data_ = reinterpret_cast<Pixel32*>(malloc(other.dataSizeBytes()));
		if (data_ == nullptr) {
			size_ = Vec2T<i64>{ 0 };
			ASSERT_NOT_REACHED();
			return *this;
		}
	}
	size_ = other.size_;
	memcpy(data_, other.data_, other.dataSizeBytes());
	return *this;
}

Image32& Image32::operator=(Image32&& other) noexcept {
	size_ = other.size_;
	data_ = other.data_;
	other.data_ = nullptr;
	return *this;
}

std::optional<Image32> Image32::fromFile(const char* path) {
	bool isLoadedCorrectly = false;
	const Image32 result{ path, isLoadedCorrectly };
	if (isLoadedCorrectly)
		return result;
	return std::nullopt;
}

void Image32::saveToPng(const char* path) const {
	stbi_write_png(path, static_cast<int>(size_.x), static_cast<int>(size_.y), 4, data_, static_cast<int>(size_.x * 4));
}

void Image32::copyAndResize(const Image32& other) {
	stbir_resize_uint8(reinterpret_cast<u8*>(other.data_), static_cast<int>(other.size_.x), static_cast<int>(other.size_.y), 0, reinterpret_cast<u8*>(data_), static_cast<int>(size_.x), static_cast<int>(size_.y), 0, 4);
}

Pixel32& Image32::operator()(i64 x, i64 y) {
	ASSERT(x < size_.x);
	ASSERT(y < size_.y);
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	return data_[y * size_.x + x];
}

const Pixel32& Image32::operator()(i64 x, i64 y) const {
	return const_cast<Image32*>(this)->operator()(x, y);
}

Vec2T<i64> Image32::size() const  {
	return size_;
}

i64 Image32::width() const {
	return size_.x;
}

i64 Image32::height() const {
	return size_.y;
}

Pixel32* Image32::data() {
	return data_;
}

const Pixel32* Image32::data() const {
	return data_;
}

usize Image32::dataSizeBytes() const {
	return 4 * size_.x * size_.y;
}

usize Image32::pixelCount() const {
	return size_.x * size_.y;
}

Pixel32* Image32::begin() {
	return data_;
}

Pixel32* Image32::end() {
	return data_ + size_.x * size_.y;
}

const Pixel32* Image32::cbegin() const {
	return data_;
}

const Pixel32* Image32::cend() const {
	return data_ + size_.x * size_.y;
}

auto Image32::indexed() -> Image32::IndexedPixelRange {
	return IndexedPixelRange{ *this };
}


Pixel32::Pixel32(u8 r, u8 g, u8 b, u8 a)
	: r{ r }
	, g{ g }
	, b{ b }
	, a{ a } {}

Pixel32::Pixel32(u8 v, u8 a)
	: r{ v }
	, g{ v }
	, b{ v }
	, a{ a } {}

Pixel32::Pixel32(const Vec4& color)
	: r{ static_cast<u8>(std::clamp(color.x * 255.0f, 0.0f, 255.0f)) }
	, g{ static_cast<u8>(std::clamp(color.y * 255.0f, 0.0f, 255.0f)) }
	, b{ static_cast<u8>(std::clamp(color.z * 255.0f, 0.0f, 255.0f)) }
	, a{ 255 } {}

auto Image32::IndexedPixelRange::begin() -> IndexedPixelIterator {
	return IndexedPixelIterator{ .pos = Vec2T<i64>{ 0 }, .data = image.data_, .rowWidth = static_cast<i64>(image.size_.x) };
}

auto Image32::IndexedPixelRange::end() -> IndexedPixelIterator {
	return IndexedPixelIterator{ .pos = Vec2T<i64>{ 0 }, .data = image.data_ + image.size_.y * image.size_.x, .rowWidth = static_cast<i64>(image.size_.x) };
}
