#include <Engine/Graphics/Shader.hpp>
#include <Utils/FileIo.hpp>
#include <Log/Log.hpp>
#include <glad/glad.h>
#include <filesystem>
#include <format>

std::expected<Shader, Shader::Error> Shader::compile(std::string_view path, ShaderType type) {
	const auto handle = glCreateShader(static_cast<GLenum>(type));
	auto source = preprocess(path);
	if (!source.has_value()) {
		return std::unexpected(Error{ Error::Type::PREPROCESS, std::move(source.error().message) });
	}
	const char* src = source->c_str();
	const auto length = static_cast<GLint>(source->length());
	glShaderSource(handle, 1, &src, &length);
	glCompileShader(handle);
	GLint status;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		char infoLog[512];
		glGetShaderInfoLog(handle, sizeof(infoLog), nullptr, infoLog);
		return std::unexpected(Error{ Error::Type::COMPILE, infoLog });
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

std::expected<std::string, Shader::PreprocessError> Shader::preprocess(std::string_view path, int depth) {
	if (depth > 5) {
		// TODO: Maybe use std::variant error type.
		LOG_FATAL("max include depth exceeded");
	}
	auto source = tryLoadStringFromFile(path);
	if (!source.has_value()) {
		return std::unexpected(Shader::PreprocessError{ std::format("cannot open '{}'", path) });
	}
	std::string_view sourceView = *source;
	auto sourceFolder = std::filesystem::path(path).parent_path();

	size_t offset = 0;
	std::vector<std::string> parts;
	for (;;) {
		std::string_view includeString = "#include \"";
		auto pathStart = sourceView.find(includeString, offset);
		if (pathStart == std::string::npos)
			break;
		pathStart += includeString.size();

		auto pathEnd = sourceView.find('"', pathStart);
		if (pathEnd == std::string::npos)
			break;

		auto pathRelativeToSourceFolder = sourceView.substr(pathStart, pathEnd - pathStart);
		auto pathRelativeToWorkspace = sourceFolder / pathRelativeToSourceFolder;
		auto pathString = pathRelativeToWorkspace.string();

		parts.push_back(std::string(sourceView.substr(offset, pathStart - includeString.size() - offset)));
		auto optSource = preprocess(pathString, depth + 1);
		if (!optSource.has_value()) {
			return std::unexpected(std::move(optSource.error()));
		}
		parts.push_back(*optSource);

		offset = pathEnd + 1;
	}

	if (parts.empty())
		return std::move(*source);

	parts.push_back(std::string(sourceView.substr(offset)));

	std::string output;
	for (const auto& part : parts)
		output += part;
	return output;
}