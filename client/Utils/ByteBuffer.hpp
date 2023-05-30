#pragma once

#include <stdlib.h>

class ByteBuffer
{
public:
	// Default new alignment is equal to __STDCPP_DEFAULT_NEW_ALIGNMENT__.
	ByteBuffer(size_t size);
	ByteBuffer(size_t size, size_t alignment);
	~ByteBuffer();

	ByteBuffer(const ByteBuffer&) = delete;
	ByteBuffer& operator= (const ByteBuffer&) = delete;

	ByteBuffer(ByteBuffer&&) noexcept;
	ByteBuffer& operator= (ByteBuffer&&) noexcept;

	template<typename T>
	T* dataAs();
	template<typename T>
	const T* dataAs() const;

	char* data();
	const char* data() const;

	size_t size() const;

private:
	char* m_data;
	size_t m_size;
};

template<typename T>
T* ByteBuffer::dataAs()
{
	static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	reinterpret_cast<T*>(m_data);
}

template<typename T>
const T* ByteBuffer::dataAs() const
{
	static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	reinterpret_cast<T*>(m_data);
}
