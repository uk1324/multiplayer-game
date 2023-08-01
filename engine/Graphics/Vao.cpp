#include <engine/Graphics/Vao.hpp>
#include <glad/glad.h>

Vao::~Vao() {
	glDeleteVertexArrays(1, &m_handle);
}

Vao Vao::generate() {
	uint32_t handle;
	glGenVertexArrays(1, &handle);
	return Vao(handle);
}

Vao Vao::null() {
	return Vao(NULL);
}

Vao::Vao(Vao&& other) noexcept
	: m_handle(other.m_handle) {
	other.m_handle = NULL;
}

Vao& Vao::operator=(Vao&& other) noexcept {
	glDeleteVertexArrays(1, &m_handle);
	m_handle = other.m_handle;
	other.m_handle = NULL;
	return *this;
}

uint32_t Vao::handle() const {
	return m_handle;
}

void Vao::setAttribute(u32 index, ShaderDataType dataType, u32 dataTypeCountPerVertex, intptr_t offset, usize stride, bool normalize) {
	bind();
	boundVaoSetAttribute(index, dataType, dataTypeCountPerVertex, offset, stride, normalize);
}

void Vao::setIntAttribute(u32 index, ShaderDataType dataType, u32 dataTypeCountPerVertex, intptr_t offset, usize stride) {
	bind();
	boundVaoSetIntAttribute(index, dataType, dataTypeCountPerVertex, offset, stride);
}

void Vao::bind() const
{
	glBindVertexArray(m_handle);
}

void Vao::unbind() {
	glBindVertexArray(NULL);
}

Vao::Vao(uint32_t handle)
	: m_handle(handle) {}

void boundVaoSetAttribute(u32 index, ShaderDataType dataType, u32 dataTypeCountPerVertex, intptr_t offset, usize stride, bool normalize) {
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

void boundVaoSetIntAttribute(u32 index, ShaderDataType dataType, u32 dataTypeCountPerVertex, intptr_t offset, usize stride) {
	glVertexAttribIPointer(
		index,
		dataTypeCountPerVertex,
		static_cast<GLenum>(dataType),
		stride,
		reinterpret_cast<void*>(offset)
	);
	glEnableVertexAttribArray(index);
}
