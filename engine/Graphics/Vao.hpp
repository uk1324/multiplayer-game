#pragma once

#include <engine/Graphics/Vbo.hpp>

enum class ShaderDataType : u32 {
	Byte = 0x1400,
	UnsignedByte = 0x1401,
	Short = 0x1402,
	UnsignedShort = 0x1403,
	Int = 0x1404,
	UnsignedInt = 0x1405,
	Float = 0x1406,
};

// https://stackoverflow.com/questions/2207320/can-i-forbid-calling-static-methods-on-object-instance
void boundVaoSetAttribute(u32 index, ShaderDataType dataType, u32 dataTypeCountPerVertex, intptr_t offset, usize stride, bool normalize = false);
void boundVaoSetIntAttribute(u32 index, ShaderDataType dataType, u32 dataTypeCountPerVertex, intptr_t offset, usize stride);

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

	void setAttribute(u32 index, ShaderDataType dataType, u32 dataTypeCountPerVertex, intptr_t offset, usize stride, bool normalize = false);
	void setIntAttribute(u32 index, ShaderDataType dataType, u32 dataTypeCountPerVertex, intptr_t offset, usize stride);

	void bind() const;
	static void unbind();

private:
	Vao(u32 handle);

	u32 m_handle;
};
