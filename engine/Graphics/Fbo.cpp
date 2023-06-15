#include <Graphics/Fbo.hpp>
#include <glad/glad.h>

Fbo::Fbo(uint32_t handle)
	: m_handle(handle)
{}

Fbo::~Fbo()
{
	glDeleteFramebuffers(1, &m_handle);
}

Fbo::Fbo(Fbo&& other) noexcept
	: m_handle(other.m_handle)
{
	other.m_handle = NULL;
}

Fbo& Fbo::operator=(Fbo&& other) noexcept
{
	m_handle = other.m_handle;
	other.m_handle = NULL;
	return *this;
}

// GL_FRAMEBUFFER binds this as the Fbo for both reading and writing.
// Using GL_READ_FRAMBUFFER or GL_DRAW_FRAMEBUFFER allows for having 2 different Fbos bound at the same time for different purposes.
void Fbo::bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void Fbo::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);
}

uint32_t Fbo::handle() const
{
	return m_handle;
}

Fbo Fbo::generate()
{
	uint32_t handle;
	glGenFramebuffers(1, &handle);
	return Fbo(handle);
}

Fbo Fbo::null()
{
	return Fbo(NULL);
}
