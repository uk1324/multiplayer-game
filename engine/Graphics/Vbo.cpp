#include <Engine/Graphics/Vbo.hpp>

#include <glad/glad.h>

Vbo::Vbo(usize dataByteSize) {
	glGenBuffers(1, &handle_);
	bind();
	glBufferData(GL_ARRAY_BUFFER, dataByteSize, nullptr, GL_DYNAMIC_DRAW);
}

Vbo::Vbo(const void* data, usize dataByteSize) {
	glGenBuffers(1, &handle_);
	bind();
	glBufferData(GL_ARRAY_BUFFER, dataByteSize, data, GL_STATIC_DRAW);
}

Vbo::~Vbo() {
 	glDeleteBuffers(1, &handle_);
}

Vbo Vbo::generate() {
	u32 handle;
	glGenBuffers(1, &handle);
	return Vbo(handle);
}

Vbo::Vbo(Vbo&& other) noexcept
	: handle_(other.handle_) {
	other.handle_ = NULL;
}

Vbo& Vbo::operator=(Vbo&& other) noexcept
{
	glDeleteBuffers(1, &handle_);
	handle_ = other.handle_;
	other.handle_ = NULL;
	return *this;
}

void Vbo::setData(intptr_t offset, const void* data, usize dataByteSize) {
	bind();
	boundVboSetData(offset, data, dataByteSize);
}

void Vbo::allocateData(const void* data, usize dataByteSize) {
	bind();
	boundVboAllocateData(data, dataByteSize);
}

void Vbo::bind() const {
	glBindBuffer(GL_ARRAY_BUFFER, handle_);
}

void Vbo::bindAsIndexBuffer() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_);
}

const GLuint Vbo::handle() const {
	return handle_;
}

Vbo::Vbo(u32 handle)
	: handle_(handle)
{}

void boundVboSetData(intptr_t offset, const void* data, usize dataByteSize) {
	glBufferSubData(GL_ARRAY_BUFFER, offset, dataByteSize, data);
}

void boundVboAllocateData(const void* data, usize dataByteSize) {
	glBufferData(GL_ARRAY_BUFFER, dataByteSize, data, GL_DYNAMIC_DRAW);
}
