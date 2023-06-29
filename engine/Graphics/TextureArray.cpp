#include <Engine/Graphics/TextureArray.hpp>
#include <Log/Log.hpp>
#include <stb_image/stb_image.h>

#include <memory>

TextureArray::TextureArray(uint32_t handle)
	: m_handle(handle)
{}

TextureArray::TextureArray(size_t width, size_t height, std::vector<std::string_view> textures)
{
	std::unique_ptr<uint32_t[]> data(new uint32_t[textures.size() * (width * height)]);

	for (size_t i = 0; i < textures.size(); i++)
	{
		int imageWidth, imageHeight, channelCount;
		static constexpr int RGBA_CHANNEL_COUNT = 4;
		uint32_t* imageData = reinterpret_cast<uint32_t*>(
			stbi_load(textures[i].data(), &imageWidth, &imageHeight, &channelCount, RGBA_CHANNEL_COUNT)
		);

		if (imageData == nullptr)
		{
			LOG_FATAL("failed to load file %s", textures[i].data());
		}

		if ((imageWidth != width) || (imageHeight != height))
		{
			LOG_FATAL("array texture width doesn't match size (width: %d, height: %d)", imageWidth, imageHeight);
		}
		/*uint32_t* imageOffset = data.get() + (i * (width * height));*/
		uint32_t* imageOffset = data.get() + (i * (width * height));
		// TODO use memcpy here
		for (size_t i = 0; i < (width * height); i++)
		{
			imageOffset[i] = imageData[i];
		}
		stbi_image_free(imageData);
	}

	static constexpr GLsizei MIPMAP_LEVEL = 0;
	glGenTextures(1, &m_handle);
	bind();

	glTexImage3D(TARGET, MIPMAP_LEVEL, GL_RGBA, width, height, textures.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());

	//glTexStorage3D(TARGET, MIPMAP_LEVEL, GL_RGBA8, width, height, textures.size());

	glTexParameteri(TARGET, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(TARGET, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(TARGET, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameterf(TARGET, GL_TEXTURE_MAX_LOD, 21000);
	//glTexParameterf(TARGET, GL_TEXTURE_MIN_LOD, -21000);

	glTexParameterf(TARGET, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

	glGenerateMipmap(TARGET);
}

TextureArray::TextureArray(size_t width, size_t height, std::string_view texturePath)
{
	int imageWidth, imageHeight, channelCount;
	static constexpr int RGBA_CHANNEL_COUNT = 4;
	//STBI_rgb_alpha
	uint8_t* data = stbi_load(texturePath.data(), &imageWidth, &imageHeight, &channelCount, RGBA_CHANNEL_COUNT);
	
	if (data == nullptr)
	{
		LOG_FATAL("failed to load file %s", texturePath.data());
	}
	if ((imageWidth % width != 0) || (imageHeight % height != 0))
	{
		LOG_FATAL("array texture width doesn't match size (width: %d, height: %d)", imageWidth, imageHeight);
	}

	static constexpr GLsizei MIPMAP_LEVEL = 0;


	glGenTextures(1, &m_handle);
	bind();
	// TODO maybe use glTexSubImage3D;
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, imageWidth);

	glTexStorage3D(TARGET, 1, GL_RGBA8, width, height, (imageWidth / width) * (imageHeight / height));

	//https://stackoverflow.com/questions/25349955/how-to-use-opengl-array-texture
	// https://gist.github.com/tilkinsc/63fe3905e139c43a013b6c6996d068ba

	GLint a;
	//glGetIntegerv(GL_UNPACK_ROW_LENGTH, &a);
	//LOG_DEBUG("%d", a);
	//glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &a);
	//LOG_DEBUG("%d", a);

	glTexParameteri(TARGET, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(TARGET, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(TARGET, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, imageWidth);
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, imageHeight);

	for (GLsizei x = 0; x < (imageWidth / width); x++) {
		for (GLsizei y = 0; y < (imageHeight / height); y++) {

			glTexSubImage3D(
				GL_TEXTURE_2D_ARRAY, 0,
				0, 0, (imageWidth / width) * y + x,
				width, height, 1,
				GL_RGBA, GL_UNSIGNED_BYTE,
				data + (y * height * imageWidth + x * width) * 4);
		}
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

	//glPixelStorei(GL_UNPACK_ROW_LENGTH, imageWidth);
	//glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, imageHeight);


	//glTexImage3D(TARGET, MIPMAP_LEVEL, GL_RGBA, width, height, (imageWidth / width) * (imageHeight / height), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);


	//bind();

	//glTexParameteri(TARGET, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(TARGET, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(TARGET, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	//glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//int subWidth = imageWidth / width;
	//int subHeight = imageHeight / height;

	//glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA, subWidth, subHeight, 12);

	//glPixelStorei(GL_UNPACK_ROW_LENGTH, imageWidth);

	//for (int iRow = 0; iRow < 4; ++iRow) {
	//	for (int iCol = 0; iCol < 3; ++iCol) {
	//		glTexSubImage3D(
	//			GL_TEXTURE_2D_ARRAY, 0,
	//			0, 0, 3 * iRow + iCol,
	//			subWidth, subHeight, 1,
	//			GL_RGBA, GL_UNSIGNED_BYTE,
	//			data + (iRow * subHeight * imageWidth + iCol * subWidth) * 4);
	//	}
	//}


	//glTexStorage3D(TARGET, MIPMAP_LEVEL, GL_RGBA8, width, height, textures.size());
	//glTexParameterf(TARGET, GL_TEXTURE_MAX_LOD, 21000);
	//glTexParameterf(TARGET, GL_TEXTURE_MIN_LOD, -21000);

	// glTexParameterf(TARGET, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

	glGenerateMipmap(TARGET);

}

TextureArray::~TextureArray()
{
	glDeleteTextures(1, &m_handle);
}

TextureArray::TextureArray(TextureArray&& other) noexcept
	: m_handle(other.m_handle)
{
	other.m_handle = NULL;
}

TextureArray& TextureArray::operator=(TextureArray&& other) noexcept
{
	m_handle = other.m_handle;
	other.m_handle = NULL;
	return *this;
}

GLuint TextureArray::handle() const
{
	return m_handle;
}

void TextureArray::bind() const
{
	glBindTexture(TARGET, m_handle);
}

TextureArray TextureArray::null()
{
	return TextureArray(NULL);
}

TextureArray TextureArray::incomplete()
{
	uint32_t handle;
	glGenTextures(1, &handle);
	return TextureArray(handle);
}
