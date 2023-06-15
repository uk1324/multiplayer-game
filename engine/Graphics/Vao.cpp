#include <Engine/Graphics/Vao.hpp>

#include <stddef.h>

BufferLayout::BufferLayout(ShaderDataType dataType, uint32_t dataTypeCountPerVertex, intptr_t offset, size_t stride, bool isNormalized)
	: dataType(dataType)
	, dataTypeCountPerVertex(dataTypeCountPerVertex)
	, offset(offset)
	, stride(stride)
	, isNormalized(isNormalized)
{}

IntBufferLayout::IntBufferLayout(ShaderDataType dataType, uint32_t dataTypeCountPerVertex, intptr_t offset, size_t stride)
	: dataType(dataType)
	, dataTypeCountPerVertex(dataTypeCountPerVertex)
	, offset(offset)
	, stride(stride)
{}

Vao::Vao()
{
	glGenVertexArrays(1, &m_handle);
}

Vao::~Vao()
{
	glDeleteVertexArrays(1, &m_handle);
}

Vao::Vao(Vao&& other) noexcept
	: m_handle(other.m_handle)
{
	other.m_handle = NULL;
}

Vao& Vao::operator=(Vao&& other) noexcept
{
	glDeleteVertexArrays(1, &m_handle);
	m_handle = other.m_handle;
	other.m_handle = NULL;
	return *this;
}

uint32_t Vao::handle() const
{
	return m_handle;
}

void Vao::setAttribute(uint32_t index, const BufferLayout& layout)
{
	glVertexAttribPointer(
		index,
		layout.dataTypeCountPerVertex,
		static_cast<GLenum>(layout.dataType),
		layout.isNormalized,
		layout.stride, 
		reinterpret_cast<void*>(layout.offset)
	);
	glEnableVertexAttribArray(index);
}

void Vao::setAttribute(uint32_t index, const IntBufferLayout& layout)
{
	glVertexAttribIPointer(
		index,
		layout.dataTypeCountPerVertex,
		static_cast<GLenum>(layout.dataType),
		layout.stride,
		reinterpret_cast<void*>(layout.offset)
	);
	glEnableVertexAttribArray(index);
}

void Vao::setAttribute(uint32_t index, ShaderDataType dataType, uint32_t dataTypeCountPerVertex, bool normalize, size_t stride, intptr_t offset)
{
	glEnableVertexAttribArray(index);
	glVertexAttribPointer(
		index,
		dataTypeCountPerVertex,
		static_cast<GLenum>(dataType),
		normalize,
		stride,
		reinterpret_cast<void*>(offset)
	);
}

void Vao::setIntAttribute(uint32_t index, ShaderDataType dataType, uint32_t dataTypeCountPerVertex, size_t stride, intptr_t offset)
{
	glVertexAttribIPointer(
		index,
		dataTypeCountPerVertex,
		static_cast<GLenum>(dataType),
		stride,
		reinterpret_cast<void*>(offset)
	);
	glEnableVertexAttribArray(index);
}

void Vao::bind() const
{
	glBindVertexArray(m_handle);
}
