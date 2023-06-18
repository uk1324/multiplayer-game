#pragma once

#include <stdint.h>

class Ibo
{
public:
	Ibo();
	Ibo(const uint32_t* indices, size_t count);
	~Ibo();

	Ibo(const Ibo&) = delete;
	Ibo& operator= (const Ibo&) = delete;

	Ibo(Ibo&& other) noexcept;
	Ibo& operator= (Ibo&& other) noexcept;

	void bind();
	static void unbind();

private:
	uint32_t m_handle;
};
