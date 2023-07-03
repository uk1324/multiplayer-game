#include <Engine/Graphics/ShaderProgram.hpp>
#include <Log/Log.hpp>
#include <glad/glad.h>
#include <sstream>

std::expected<ShaderProgram, ShaderProgram::Error> ShaderProgram::compile(std::string_view vertexPath, std::string_view fragmentPath) {
	auto vertex = Shader::compile(vertexPath, ShaderType::Vertex);
	auto fragment = Shader::compile(fragmentPath, ShaderType::Fragment);
	if (!vertex.has_value() || !fragment.has_value()) {
		return std::unexpected(Error{ 
			.vertexError = !vertex.has_value() ? std::optional(std::move(vertex.error())) : std::nullopt,
			.fragmentError = !fragment.has_value() ? std::optional(std::move(fragment.error())) : std::nullopt,
		});
	}

	ShaderProgram program(glCreateProgram());
	program.addShader(*vertex);
	program.addShader(*fragment);
	auto linkerError = program.link();
	if (linkerError.has_value()) {
		return std::unexpected(Error{ .linkerErrorMessage = std::move(*linkerError) });
	}
	return program;
}

ShaderProgram ShaderProgram::create(std::string_view vertexPath, std::string_view fragmentPath) {
	auto shader = compile(vertexPath, fragmentPath);
	if (shader.has_value()) {
		return std::move(*shader);
	}
	LOG_FATAL("failed to compile vert = '%s' frag = '%s': %s", vertexPath.data(), fragmentPath.data(), shader.error().toSingleMessage().c_str());
}

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(handle_);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
	: handle_(other.handle_) {
	other.handle_ = NULL;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
	glDeleteProgram(handle_);
	handle_ = other.handle_;
	other.handle_ = NULL;
	m_cachedUniformLocations = std::move(other.m_cachedUniformLocations);
	return *this;
}

void ShaderProgram::addShader(const Shader& shader) {
	glAttachShader(handle_, shader.handle());
}

std::optional<std::string> ShaderProgram::link() {
	glLinkProgram(handle_);
	GLint status;
	glGetProgramiv(handle_, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		char infoLog[512];
		glGetProgramInfoLog(handle_, sizeof(infoLog), nullptr, infoLog);
		return infoLog;
	}
	return std::nullopt;
}

void ShaderProgram::use() {
	glUseProgram(handle_);
}

// Using glProgramUniform instead of glUniform might be sometimes faster because it doesn't need a glUsePgoram call before it.
void ShaderProgram::set(std::string_view name, const Vec2& vec) {
	glProgramUniform2fv(handle_, getUniformLocation(name.data()), 1, vec.data());
}

void ShaderProgram::set(std::string_view name, const Vec3& vec) {
	glProgramUniform3fv(handle_, getUniformLocation(name.data()), 1, vec.data());
}

void ShaderProgram::set(std::string_view name, const Vec4& vec) {
	glProgramUniform4fv(handle_, getUniformLocation(name.data()), 1, vec.data());
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

void ShaderProgram::set(std::string_view name, int32_t value) {
	glProgramUniform1i(handle_, getUniformLocation(name.data()), value);
}

void ShaderProgram::set(std::string_view name, uint32_t value) {
	glProgramUniform1ui(handle_, getUniformLocation(name.data()), value);
}

// TODO make a utility function that takes the actual texture and an index and call glActiveTexture.
void ShaderProgram::setTexture(std::string_view name, int value) {
	set(name, value);
}

void ShaderProgram::set(std::string_view name, float value) {
	glProgramUniform1f(handle_, getUniformLocation(name.data()), value);
}

void ShaderProgram::set(std::string_view name, bool value) {
	// There is no special function for bools.
	set(name, static_cast<i32>(value));
}

void ShaderProgram::set(std::string_view name, const Mat3x2& value) {
	glProgramUniformMatrix3x2fv(handle_, getUniformLocation(name), 1, false, reinterpret_cast<const float*>(value.m));
}

void ShaderProgram::set(std::string_view name, std::span<const Vec2> vecs) {
	glProgramUniform2fv(handle_, getUniformLocation(name.data()), vecs.size(), reinterpret_cast<const float*>(vecs.data()));
}

//void ShaderProgram::setColor(std::string_view name, const Color& value)
//{
//	glProgramUniform4fv(m_handle, getUniformLocation(name.data()), 1, value.data());
//}

GLuint ShaderProgram::handle() const {
	return handle_;
}

ShaderProgram::ShaderProgram(u32 handle) 
	: handle_(handle) {
}

int ShaderProgram::getUniformLocation(std::string_view name) {
	// Can't assume that the string_view data won't get destroyed.
	std::string uniformName(name);
	auto location = m_cachedUniformLocations.find(uniformName);
	if (location == m_cachedUniformLocations.end())
	{
		int location = glGetUniformLocation(handle_, uniformName.c_str());
		if (location == -1)
			LOG_WARNING("trying to set variable '%.*s', which doesn't exist", name.max_size(), name.data());

		m_cachedUniformLocations[std::move(uniformName)] = location;
		return location;
	}
	return location->second;
}

std::string ShaderProgram::Error::toSingleMessage() const {
	std::stringstream errorMessage;
	if (vertexError.has_value()) {
		errorMessage << "vert error: " << vertexError->message << '\n';
	}
	if (fragmentError.has_value()) {
		errorMessage << "frag error: " << fragmentError->message << '\n';
	}
	if (linkerErrorMessage.has_value()) {
		errorMessage << "linker error: " << *linkerErrorMessage << '\n';
	}
	return errorMessage.str();
}
