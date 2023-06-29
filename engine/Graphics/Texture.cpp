#include <Engine/Graphics/Texture.hpp>
#include <Log/Log.hpp>
#include <stb_image/stb_image.h>
#include <glad/glad.h>

Texture::Texture(uint32_t handle)
	: handle_(handle)
{}

static constexpr u32 TARGET = GL_TEXTURE_2D;

Texture::Texture(const Image32& img, const Settings& settings) {
	glGenTextures(1, &handle_);
	bind();
	glTexImage2D(TARGET, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<const void*>(img.data()));

	glTexParameteri(TARGET, GL_TEXTURE_WRAP_S, static_cast<GLint>(settings.wrapS));
	glTexParameteri(TARGET, GL_TEXTURE_WRAP_T, static_cast<GLint>(settings.wrapT));
	glTexParameteri(TARGET, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(settings.minFilter));
	glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(settings.magFilter));

	glGenerateMipmap(TARGET);
}

Texture::Texture(std::string_view path) {
	int width, height, channelCount;
	static constexpr int REQUIRED_CHANNEL_COUNT = 4;
	void* data = stbi_load(path.data(), &width, &height, &channelCount, REQUIRED_CHANNEL_COUNT);
	if (data == nullptr)
	{
		LOG_FATAL("failed to read texture file %s", path.data());
	}

	glGenTextures(1, &handle_);
	bind();
	glTexImage2D(TARGET, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameteri(TARGET, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(TARGET, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(TARGET, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenerateMipmap(TARGET);

	stbi_image_free(data);
}

Texture::~Texture()
{
	glDeleteTextures(1, &handle_);
}

Texture::Texture(Texture&& other) noexcept
	: handle_(other.handle_)
{
	other.handle_ = NULL;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
	glDeleteTextures(1, &handle_);
	handle_ = other.handle_;
	other.handle_ = NULL;
	return *this;
}

void Texture::bind() const
{
	glBindTexture(TARGET, handle_);
}

GLuint Texture::handle() const
{
	return handle_;
}

Texture Texture::pixelArt(const char* path)
{
	auto texture = Texture::generate();
	texture.bind();

	Image32 image(path);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenerateMipmap(GL_TEXTURE_2D);

	return texture;
}

Texture Texture::generate()
{
	uint32_t handle;
	glGenTextures(1, &handle);
	return Texture(handle);
}

Texture Texture::null()
{
	return Texture(NULL);
}
