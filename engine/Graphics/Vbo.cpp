#include <Engine/Graphics/Vbo.hpp>

#include <glad/glad.h>

Vbo::Vbo()
// TODO: should be explicit.
	: m_handle(NULL)
{}

Vbo::Vbo(size_t dataByteSize)
{
	glGenBuffers(1, &m_handle);
	bind();
	glBufferData(GL_ARRAY_BUFFER, dataByteSize, nullptr, GL_DYNAMIC_DRAW);
}

Vbo::Vbo(const void* data, size_t dataByteSize)
{
	glGenBuffers(1, &m_handle);
	bind();
	glBufferData(GL_ARRAY_BUFFER, dataByteSize, data, GL_STATIC_DRAW);
}

Vbo::~Vbo()
{
 	glDeleteBuffers(1, &m_handle);
}

Vbo Vbo::generate()
{
	Vbo vbo;
	glGenBuffers(1, &vbo.m_handle);
	return vbo;
}

Vbo::Vbo(Vbo&& other) noexcept
	: m_handle(other.m_handle)
{
	other.m_handle = NULL;
}

Vbo& Vbo::operator=(Vbo&& other) noexcept
{
	glDeleteBuffers(1, &m_handle);
	m_handle = other.m_handle;
	other.m_handle = NULL;
	return *this;
}

void Vbo::setData(intptr_t offset, const void* data, size_t dataByteSize)
{
	glBufferSubData(GL_ARRAY_BUFFER, offset, dataByteSize, data);
}

void Vbo::allocateData(const void* data, size_t dataByteSize)
{
	glBufferData(GL_ARRAY_BUFFER, dataByteSize, data, GL_DYNAMIC_DRAW);
}

void Vbo::bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_handle);
}

void Vbo::bindAsIndexBuffer() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_handle);
}

const GLuint Vbo::handle() const
{
	return m_handle;
}
