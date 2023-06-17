#pragma once

#include <Engine/Graphics/Shader.hpp>
#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>
#include <engine/Math/Vec4.hpp>
#include <engine/Math/mat3x2.hpp>

#include <unordered_map>
#include <optional>

class ShaderProgram {
public:
	struct Error {
		std::string vertexErrorMessage;
		std::string fragmentErrorMessage;
		std::string linkerErrorMessage;

		std::string toSingleMessage() const;
	};

	static std::variant<ShaderProgram, Error> compile(std::string_view vertexPath, std::string_view fragmentPath);
	static ShaderProgram create(std::string_view vertexPath, std::string_view fragmentPath);
	~ShaderProgram();

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator= (const ShaderProgram& other) = delete;

	ShaderProgram(ShaderProgram&&) noexcept;
	ShaderProgram& operator= (ShaderProgram&& other) noexcept;

	// Wanted to use the constructor to pass a vector of shaders but Shader is non copyable and you can't have a pointer to a reference.
	void addShader(const Shader& shader);
	std::optional<std::string> link();

	void use();

	void set(std::string_view name, const Vec2& vec);
	void set(std::string_view name, const Vec3& vec);
	void set(std::string_view name, const Vec4& vec);
	void set(std::string_view name, int32_t value);
	void set(std::string_view name, uint32_t value);
	void setTexture(std::string_view name, int value);
	void set(std::string_view name, float value);
	void set(std::string_view name, bool value);
	void set(std::string_view name, const Mat3x2& value);

	u32 handle() const;

private:
	ShaderProgram(u32 handle);
	// This isn't the best way to cache uniforms.
	// A better way would to to parse the glsl files and extract the variables declarations.
	// ^ (if you really tried you could make this solution also not require map lookup)
	// (doing this would be pretty hard because of struct declarations and other thing like iterface blocks).
	// An even faster solution would be to store the locations in variables so no hashmap lookup is required.
	// You could inherit from ShaderProgram and create a struct locations in the class and initialize them on creation.
	// That would require a lot of manual work and would be prone to errors so it isn't worth it.
	int getUniformLocation(std::string_view name);

private:
	u32 handle_;

	std::unordered_map<std::string, int> m_cachedUniformLocations;
};
