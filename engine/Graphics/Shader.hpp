#pragma once

#include <Types.hpp>
#include <string_view>
#include <variant>

enum class ShaderType {
	Vertex = 0x8B31,
	Fragment = 0x8B30,
	Geometry = 0x8DD9,
};

class Shader {
public:
	struct Error {
		std::string message;
	};
	static std::variant<Shader, Error> compile(std::string_view path, ShaderType type);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator= (const Shader&) = delete;

	Shader(Shader&& other) noexcept;
	Shader& operator= (Shader&& other) noexcept;

	u32 handle() const;

private:
	Shader(u32 handle);
	static std::string preprocess(std::string_view path, int depth = 0);

	u32 handle_;
};
