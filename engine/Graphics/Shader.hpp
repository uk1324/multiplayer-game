#pragma once

#include <Types.hpp>
#include <string_view>
#include <expected>

enum class ShaderType {
	Vertex = 0x8B31,
	Fragment = 0x8B30,
	Geometry = 0x8DD9,
};

class Shader {
public:
	struct Error {
		enum class Type {
			// Maybe slipt preprocess to cannot open file and file doesn't exist then try to reload again if cannot open file but the file exists. This might make it go into a long loop if something is actually blocking the reading of the file.
			PREPROCESS,
			COMPILE
		};
		Type type;
		std::string message;
	};
	static std::expected<Shader, Error> compile(std::string_view path, ShaderType type);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator= (const Shader&) = delete;

	Shader(Shader&& other) noexcept;
	Shader& operator= (Shader&& other) noexcept;

	u32 handle() const;

private:
	Shader(u32 handle);

	struct PreprocessError {
		std::string message;
	};
	static std::expected<std::string, PreprocessError> preprocess(std::string_view path, int depth = 0);

	u32 handle_;
};
