#pragma once

#include <glad/glad.h>

#include <string_view>

enum class ShaderType
{
	Vertex = GL_VERTEX_SHADER,
	Fragment = GL_FRAGMENT_SHADER,
	Geometry = GL_GEOMETRY_SHADER,
};

class Shader
{
public:
	explicit Shader(std::string_view path, ShaderType type);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator= (const Shader&) = delete;

	Shader(Shader&& other) noexcept;
	Shader& operator= (Shader&& other) noexcept;

	GLuint handle() const;

private:
	std::string preprocess(std::string_view path, int depth);

private:
	GLuint m_handle;
};
