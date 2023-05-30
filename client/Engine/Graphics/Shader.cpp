#include <Engine/Graphics/Shader.hpp>
#include <Utils/FileIo.hpp>
#include <Log/Log.hpp>

#include <filesystem>

Shader::Shader(std::string_view path, ShaderType type)
{
	m_handle = glCreateShader(static_cast<GLenum>(type));

	std::string source = preprocess(path, 0);
	const char* src = source.c_str();
	GLint length = static_cast<GLint>(source.length());
	glShaderSource(m_handle, 1, &src, &length);
	glCompileShader(m_handle);
	GLint status;
	glGetShaderiv(m_handle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		char infoLog[512];
		glGetShaderInfoLog(m_handle, sizeof(infoLog), nullptr, infoLog);
		LOG_FATAL("failed to compile shader %s\n%s\n", path.data(), infoLog);
	}
}

Shader::Shader(Shader&& other) noexcept
	: m_handle(other.m_handle)
{
	other.m_handle = NULL;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
	glDeleteShader(m_handle);
	m_handle = other.m_handle;
	other.m_handle = NULL;
	return *this;
}

Shader::~Shader()
{
	glDeleteShader(m_handle);
}

GLuint Shader::handle() const
{
	return m_handle;
}

std::string Shader::preprocess(std::string_view path, int depth)
{
	if (depth > 5)
	{
		LOG_FATAL("max include depth exceeded");
	}
	auto sourceString = stringFromFile(path);
	std::string source = sourceString;
	auto sourceFolder = std::filesystem::path(path).parent_path();

	size_t offset = 0;
	std::vector<std::string> parts;
	for (;;) 
	{
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