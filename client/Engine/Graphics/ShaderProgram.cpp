#include <Engine/Graphics/ShaderProgram.hpp>
#include <Log/Log.hpp>

ShaderProgram::ShaderProgram()
	: m_handle(glCreateProgram())
{}

ShaderProgram::ShaderProgram(std::string_view vertexPath, std::string_view fragmentPath)
	: ShaderProgram()
{
	Shader vertex(vertexPath, ShaderType::Vertex);
	Shader fragment(fragmentPath, ShaderType::Fragment);
	addShader(vertex);
	addShader(fragment);
	link();
}

ShaderProgram::ShaderProgram(std::string_view vertexPath, std::string_view geometryPath, std::string_view fragmentPath)
	: ShaderProgram()
{
	Shader vertex(vertexPath, ShaderType::Vertex);
	Shader geoemetry(geometryPath, ShaderType::Geometry);
	Shader fragment(fragmentPath, ShaderType::Fragment);
	addShader(vertex);
	addShader(geoemetry);
	addShader(fragment);
	link();
}

ShaderProgram::~ShaderProgram()
{
	glDeleteProgram(m_handle);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
	: m_handle(other.m_handle)
{
	other.m_handle = NULL;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
	glDeleteProgram(m_handle);
	m_handle = other.m_handle;
	other.m_handle = NULL;
	m_cachedUniformLocations = other.m_cachedUniformLocations;
	return *this;
}

void ShaderProgram::addShader(const Shader& shader)
{
	glAttachShader(m_handle, shader.handle());
}

void ShaderProgram::link()
{
	glLinkProgram(m_handle);
	GLint status;
	glGetProgramiv(m_handle, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		char infoLog[512];
		glGetProgramInfoLog(m_handle, sizeof(infoLog), nullptr, infoLog);
		LOG_FATAL("failed to link shaders: %s", infoLog);
	}
}

void ShaderProgram::use()
{
	glUseProgram(m_handle);
}

// Using glProgramUniform instead of glUniform might be sometimes faster because it doesn't need a glUsePgoram call before it.
void ShaderProgram::setVec2(std::string_view name, const Vec2& vec)
{
	glProgramUniform2fv(m_handle, getUniformLocation(name.data()), 1, vec.data());
}

void ShaderProgram::setVec3(std::string_view name, const Vec3& vec)
{
	glProgramUniform3fv(m_handle, getUniformLocation(name.data()), 1, vec.data());
}

//void ShaderProgram::setVec3I(std::string_view name, const Vec3I& vec)
//{
//	glProgramUniform3iv(m_handle, getUniformLocation(name.data()), 1, vec.data());
//}
//
//void ShaderProgram::setMat4(std::string_view name, const Mat4& mat)
//{
//	glProgramUniformMatrix4fv(m_handle, getUniformLocation(name.data()), 1, GL_FALSE, mat.data());
//}

void ShaderProgram::setInt(std::string_view name, int32_t value)
{
	glProgramUniform1i(m_handle, getUniformLocation(name.data()), value);
}

void ShaderProgram::setUnsignedInt(std::string_view name, uint32_t value)
{
	glProgramUniform1ui(m_handle, getUniformLocation(name.data()), value);
}

// TODO make a utility function that takes the actual texture and an index and call glActiveTexture.
void ShaderProgram::setTexture(std::string_view name, int value)
{
	setInt(name, value);
}

void ShaderProgram::setFloat(std::string_view name, float value)
{
	glProgramUniform1f(m_handle, getUniformLocation(name.data()), value);
}

void ShaderProgram::setBool(std::string_view name, bool value)
{
	// There is no special function for bools.
	setInt(name, value);
}

void ShaderProgram::setMat3x2(std::string_view name, const Mat3x2& value) {
	glProgramUniformMatrix3x2fv(m_handle, getUniformLocation(name), 1, false, reinterpret_cast<const float*>(value.m));
}

//void ShaderProgram::setColor(std::string_view name, const Color& value)
//{
//	glProgramUniform4fv(m_handle, getUniformLocation(name.data()), 1, value.data());
//}

GLuint ShaderProgram::handle() const
{
	return m_handle;
}

int ShaderProgram::getUniformLocation(std::string_view name)
{
	// Can't assume that the string_view data won't get destroyed.
	std::string uniformName(name);
	auto location = m_cachedUniformLocations.find(uniformName);
	if (location == m_cachedUniformLocations.end())
	{
		int location = glGetUniformLocation(m_handle, uniformName.c_str());
		if (location == -1)
			LOG_WARNING("trying to set variable '%.*s', which doesn't exist", name.max_size(), name.data());

		m_cachedUniformLocations[std::move(uniformName)] = location;
		return location;
	}
	return location->second;
}