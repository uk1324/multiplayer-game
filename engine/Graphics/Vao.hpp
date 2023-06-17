#pragma once

#include <Engine/Graphics/Vbo.hpp>

enum class ShaderDataType : uint32_t
{
	Byte = 0x1400,
	UnsignedByte = 0x1401,
	Short = 0x1402,
	UnsignedShort = 0x1403,
	Int = 0x1404,
	UnsignedInt = 0x1405,
	Float = 0x1406,
};

struct BufferLayout
{
public:
	BufferLayout() = default;
	BufferLayout(ShaderDataType dataType, uint32_t dataTypeCountPerVertex, intptr_t offset, size_t stride, bool normalize);

public:
	ShaderDataType dataType;
	uint32_t dataTypeCountPerVertex;
	bool isNormalized;
	size_t stride;
	intptr_t offset;
};

struct IntBufferLayout
{
public:
	IntBufferLayout() = default;
	IntBufferLayout(ShaderDataType dataType, uint32_t dataTypeCountPerVertex, intptr_t offset, size_t stride);

public:
	ShaderDataType dataType;
	uint32_t dataTypeCountPerVertex;
	size_t stride;
	intptr_t offset;
};

class Vao {
public:
	~Vao();
	static Vao generate();
	static Vao null();
	Vao(const Vao&) = delete;
	Vao& operator= (const Vao&) = delete;

	Vao(Vao&& other) noexcept;
	Vao& operator= (Vao&& other) noexcept;

	uint32_t handle() const;

	// Sets the atribute of the currently bound VertexArray at index to the currently bound VertexBuffer with specified layout.
	static void setAttribute(uint32_t index, const BufferLayout& layout);
	static void setAttribute(uint32_t index, const IntBufferLayout& layout);

	static void setAttribute(
		uint32_t index,
		ShaderDataType dataType,
		uint32_t dataTypeCountPerVertex,
		bool normalize,
		size_t stride,
		intptr_t offset);

	static void setIntAttribute(
		uint32_t index,
		ShaderDataType dataType,
		uint32_t dataTypeCountPerVertex,
		size_t stride,
		intptr_t offset);

	void bind() const;

private:
	Vao(uint32_t handle);

	uint32_t m_handle;
};
