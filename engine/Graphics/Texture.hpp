#pragma once

#include <Image32.hpp>
#include <string_view>
#include <Types.hpp>

struct Texture {
	enum class Filter {
		NEAREST = 0x2600,
		LINEAR = 0x2601
	};

	enum class Wrap {
		REPEAT = 0x2901,
		CLAMP_TO_EDGE = 0x812F,
	};

	struct Settings {
		Filter magFilter = Filter::LINEAR;
		Filter minFilter = Filter::LINEAR;
		Wrap wrapS = Wrap::REPEAT;
		Wrap wrapT = Wrap::REPEAT;
	};

	Texture(uint32_t handle);
	Texture(const Image32& img, const Settings& settings);
	Texture(std::string_view path);
	~Texture();

	Texture(const Texture&) = delete;
	Texture& operator= (const Texture&) = delete;

	Texture(Texture&& other) noexcept;
	Texture& operator= (Texture&& other) noexcept;

	void bind() const;

	u32 handle() const;

	static Texture pixelArt(const char* path);
	static Texture generate();
	static Texture null();

private:
	u32 handle_;
};
