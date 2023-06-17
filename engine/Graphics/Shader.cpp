#include <Engine/Graphics/Shader.hpp>
#include <Utils/FileIo.hpp>
#include <Log/Log.hpp>
#include <glad/glad.h>
#include <filesystem>

std::variant<Shader, Shader::Error> Shader::compile(std::string_view path, ShaderType type) {
	const auto handle = glCreateShader(static_cast<GLenum>(type));
	std::string source = preprocess(path);
	const char* src = source.c_str();
	const auto length = static_cast<GLint>(source.length());
	glShaderSource(handle, 1, &src, &length);
	glCompileShader(handle);
	GLint status;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		char infoLog[512];
		glGetShaderInfoLog(handle, sizeof(infoLog), nullptr, infoLog);
		return Error{ infoLog };
	}
	return Shader(handle);
}

Shader::Shader(Shader&& other) noexcept
	: handle_(other.handle_) {
	other.handle_ = NULL;
}

Shader& Shader::operator=(Shader&& other) noexcept {
	glDeleteShader(handle_);
	handle_ = other.handle_;
	other.handle_ = NULL;
	return *this;
}

Shader::~Shader() {
	glDeleteShader(handle_);
}

GLuint Shader::handle() const {
	return handle_;
}

Shader::Shader(u32 handle)
	: handle_(handle){}

std::string Shader::preprocess(std::string_view path, int depth) {
	if (depth > 5) {
		LOG_FATAL("max include depth exceeded");
	}
	auto sourceString = stringFromFile(path);
	std::string source = sourceString;
	auto sourceFolder = std::filesystem::path(path).parent_path();

	size_t offset = 0;
	std::vector<std::string> parts;
	for (;;) {
		std::string_view includeString = "#include \"";
		auto pathStart = source.find(includeString, offset);
		if (pathStart == std::string::npos)
			break;
		pathStart += includeString.size();

		auto pathEnd = source.find('"', pathStart);
		if (pathEnd == std::string::npos)
			break;

		auto pathRelativeToSourceFolder = source.substr(pathStart, pathEnd - pathStart);
		auto pathRelativeToWorkspace = sourceFolder / pathRelativeToSourceFolder;
		auto pathString = pathRelativeToWorkspace.string();

		parts.push_back(std::string(source.substr(offset, pathStart - includeString.size() - offset)));
		parts.push_back(preprocess(pathString, depth + 1));

		offset = pathEnd + 1;
	}

	if (parts.empty())
		return sourceString;

	parts.push_back(std::string(source.substr(offset)));

	std::string output;
	for (const auto& part : parts)
		output += part;
	return output;
}