#pragma once

#include <engine/Utils/PreprocessIncludes.hpp>
#include <Types.hpp>
#include <string_view>
#include <expected>
#include <variant>

enum class ShaderType {
	Vertex = 0x8B31,
	Fragment = 0x8B30,
	Geometry = 0x8DD9,
};

class Shader {
public:
	struct CompileError {
		std::string message;
	};

	using Error = std::variant<PreprocessIncludesError, CompileError>;

	static std::expected<Shader, Error> compile(std::string_view path, ShaderType type);
	static std::expected<Shader, Error> fromSource(std::string_view source, ShaderType type);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator= (const Shader&) = delete;

	Shader(Shader&& other) noexcept;
	Shader& operator= (Shader&& other) noexcept;

	u32 handle() const;

private:
	Shader(u32 handle);

	u32 handle_;
};
	
std::ostream& operator<<(std::ostream& os, const Shader::Error& e);