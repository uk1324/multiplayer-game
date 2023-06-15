#include <Utils/ByteBuffer.hpp>

#include <new>

ByteBuffer::ByteBuffer(size_t size)
	: m_data(reinterpret_cast<char*>(operator new(size)))
	, m_size(size)
{}

ByteBuffer::ByteBuffer(size_t size, size_t alignment)
	: m_data(reinterpret_cast<char*>(operator new(size, std::align_val_t(alignment))))
	, m_size(size)
{}

ByteBuffer::~ByteBuffer()
{
	operator delete(m_data);
}

ByteBuffer::ByteBuffer(ByteBuffer&& other) noexcept
	: m_data(other.m_data)
	, m_size(other.m_size)
{
	other.m_data = nullptr;
}

ByteBuffer& ByteBuffer::operator= (ByteBuffer&& other) noexcept
{
	m_data = other.m_data;
	m_size = other.m_size;
	other.m_data = nullptr;
	return *this;
}

char* ByteBuffer::data()
{
	return m_data;
}

const char* ByteBuffer::data() const
{
	return m_data;
}

size_t ByteBuffer::size() const
{
	return m_size;
}
