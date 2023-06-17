#pragma once

#include <Image32.hpp>
#include <string_view>
#include <Types.hpp>

class Texture
{
public:
	Texture(uint32_t handle);
	Texture(const Image32& img);
	Texture(std::string_view path);
	~Texture();

	Texture(const Texture&) = delete;
	Texture& operator= (const Texture&) = delete;

	Texture(Texture&& other) noexcept;
	Texture& operator= (Texture&& other) noexcept;

	void bind() const;

	u32 handle() const;

public:
	static Texture pixelArt(const char* path);
	static Texture generate();
	static Texture null();

private:
	static constexpr u32 TARGET = 0xDE1;

private:
	u32 m_handle;
};
