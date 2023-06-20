#pragma once

#include <Types.hpp>

class Ibo {
public:
	static Ibo generate();
	Ibo(const void* data, usize dataByteSize);
	~Ibo();

	Ibo(const Ibo&) = delete;
	Ibo& operator= (const Ibo&) = delete;

	Ibo(Ibo&& other) noexcept;
	Ibo& operator= (Ibo&& other) noexcept;

	void allocateData(const void* data, usize dataByteSize);

	void bind();
	static void unbind();

private:
	Ibo(u32 handle);

	u32 handle_;
};
