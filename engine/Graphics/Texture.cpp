#include <Engine/Graphics/Texture.hpp>
#include <Log/Log.hpp>
#include <stb_image.h>

Texture::Texture(uint32_t handle)
	: m_handle(handle)
{}

Texture::Texture(const Image32& img) {
	glGenTextures(1, &m_handle);
	bind();
	glTexImage2D(TARGET, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());

	glTexParameteri(TARGET, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(TARGET, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(TARGET, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenerateMipmap(TARGET);
}

Texture::Texture(std::string_view path)
{
	int width, height, channelCount;
	static constexpr int REQUIRED_CHANNEL_COUNT = 4;
	void* data = stbi_load(path.data(), &width, &height, &channelCount, REQUIRED_CHANNEL_COUNT);
	if (data == nullptr)
	{
		LOG_FATAL("failed to read texture file %s", path.data());
	}

	glGenTextures(1, &m_handle);
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
	glDeleteTextures(1, &m_handle);
}

Texture::Texture(Texture&& other) noexcept
	: m_handle(other.m_handle)
{
	other.m_handle = NULL;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
	glDeleteTextures(1, &m_handle);
	m_handle = other.m_handle;
	other.m_handle = NULL;
	return *this;
}

void Texture::bind() const
{
	glBindTexture(TARGET, m_handle);
}

GLuint Texture::handle() const
{
	return m_handle;
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
