#include <engine/Graphics/Ibo.hpp>
#include <glad/glad.h>

Ibo Ibo::generate() {
	u32 handle;
	glGenBuffers(1, &handle);
	return Ibo(handle);
}

Ibo::Ibo(const void* data, usize dataByteSize) {
	glGenBuffers(1, &handle_);
	allocateData(data, dataByteSize);
}

Ibo::~Ibo() {
	glDeleteBuffers(1, &handle_);
}

Ibo::Ibo(Ibo&& other) noexcept
	: handle_(other.handle_) {
	other.handle_ = NULL;
}

Ibo& Ibo::operator=(Ibo&& other) noexcept {
	glDeleteBuffers(1, &handle_);
	handle_ = other.handle_;
	other.handle_ = NULL;
	return *this;
}

void Ibo::allocateData(const void* data, usize dataByteSize) {
	// GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound vao.
	// Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of vao state. 
	glBindBuffer(GL_ARRAY_BUFFER, handle_);
	glBufferData(GL_ARRAY_BUFFER, dataByteSize, data, GL_STATIC_DRAW);
}

void Ibo::bind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_);
}

void Ibo::unbind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

Ibo::Ibo(u32 handle)
	: handle_(handle) {}
