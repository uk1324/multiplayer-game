#pragma once

#include <glad/glad.h>

#include <vector>
#include <string_view>

// A texture array requires all textures to be the same size.


class TextureArray
{
public:
	TextureArray(uint32_t handle);
	TextureArray(size_t width, size_t height, size_t layers);
	TextureArray(size_t width, size_t height, std::vector<std::string_view> textures);
	TextureArray(size_t width, size_t height, std::string_view texturePath);
	~TextureArray();

	TextureArray(const TextureArray&) = delete;
	TextureArray& operator= (const TextureArray&) = delete;

	TextureArray(TextureArray&& other) noexcept;
	TextureArray& operator= (TextureArray&& other) noexcept;

	GLuint handle() const;

	void bind() const;

public:
	static TextureArray null();
	static TextureArray incomplete();

private:
	static constexpr GLenum TARGET = GL_TEXTURE_2D_ARRAY;

private:
	GLuint m_handle;
};
