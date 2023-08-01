#pragma once

#include <Types.hpp>

class Fbo
{
public:
	Fbo(uint32_t handle);
	~Fbo();

	Fbo(const Fbo&) = delete;
	Fbo& operator= (const Fbo&) = delete;

	Fbo(Fbo&& other) noexcept;
	Fbo& operator= (Fbo&& other) noexcept;

	void bind();
	static void unbind();

	uint32_t handle() const;

public:
	static Fbo generate();
	static Fbo null();

private:
	uint32_t m_handle;
};