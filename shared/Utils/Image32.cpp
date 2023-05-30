#include "Image32.hpp"
#include <Log/Log.hpp>
#include <stb_image.h>
#include <shared/Utils/Assertions.hpp>

#include <fstream>

Image32::Image32(const char* path)
{
	int channelCount;
	int width, height;
	m_data = reinterpret_cast<uint32_t*>(stbi_load(path, &width, &height, &channelCount, STBI_rgb_alpha));
	if (m_data == nullptr)
	{
		LOG_FATAL("failed to load image");
	}
	m_width = width;
	m_height = height;
}

Image32::Image32(size_t width, size_t height)
	: m_width(width)
	, m_height(height)
	, m_data(reinterpret_cast<uint32_t*>(allocate(width * height * sizeof(uint32_t))))
{}

Image32::Image32(const Image32& other)
	: m_width(other.m_width)
	, m_height(other.m_height)
	, m_data(reinterpret_cast<uint32_t*>(allocate(other.m_width * other.m_height * sizeof(uint32_t))))
{
	memcpy(m_data, other.m_data, other.m_width * other.m_height * sizeof(uint32_t));
}

Image32& Image32::operator=(const Image32& other)
{
	if (&other == this)
		return *this;

	stbi_image_free(m_data);
	m_data = reinterpret_cast<uint32_t*>(allocate(other.m_width * other.m_height * sizeof(uint32_t)));
	m_width = other.m_width;
	m_height = other.m_height;
	memcpy(m_data, other.m_data, other.m_width * other.m_height * sizeof(uint32_t));
}

Image32::Image32(Image32&& other) noexcept
	: m_width(other.m_width)
	, m_height(other.m_height)
	, m_data(other.m_data)
{
	other.m_data = nullptr;
}

Image32& Image32::operator=(Image32&& other) noexcept
{
	m_width = other.m_width;
	m_height = other.m_height;
	m_data = other.m_data;
	other.m_data = nullptr;
	return *this;
}

Image32::~Image32()
{
	stbi_image_free(m_data);
}

// Images are loaded upside down so y is flipped upside down.
void Image32::set(size_t x, size_t y, Color32 color)
{
	ASSERT(x < width());
	ASSERT(y < height());
	*reinterpret_cast<Color32*>(&m_data[(m_height - y - 1) * m_width + x]) = color;
}

Color32 Image32::get(size_t x, size_t y) const
{
	ASSERT(x < width());
	ASSERT(y < height());
	return *reinterpret_cast<Color32*>(&m_data[(m_height - y - 1) * m_width + x]);
}

size_t Image32::width() const
{
	return m_width;
}

size_t Image32::height() const
{
	return m_height;
}

uint32_t* Image32::data()
{
	return m_data;
}

const uint32_t* Image32::data() const
{
	return m_data;
}

void Image32::saveAsPpm(std::string_view path) const
{
	std::ofstream file(std::string(path), std::ios_base::binary);
	file << "P6 " << m_width << ' ' << m_height << " 255\n";
	for (uint32_t* ptr = m_data; ptr != m_data + (m_width * m_height); ptr++)
	{
		file.write(reinterpret_cast<const char*>(ptr), 3);
	}
}

void* Image32::allocate(size_t size)
{
	// Using malloc so its compatible with stbi.
	void* data = malloc(size);
	if (data == nullptr)
	{
		LOG_FATAL("failed to allocate memory for image");
	}
	return data;
}

