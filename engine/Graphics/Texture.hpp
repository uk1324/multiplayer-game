#pragma once

#include <glad/glad.h>
#include <Image32.hpp>
#include <string_view>


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

	GLuint handle() const;

public:
	static Texture pixelArt(const char* path);
	static Texture generate();
	static Texture null();

private:
	static constexpr GLenum TARGET = GL_TEXTURE_2D;

private:
	GLuint m_handle;
};
