#pragma once

#include <engine/Graphics/ShaderProgram.hpp>

struct ShaderManager {
	static void terminate();
	static void update();
	static void reloadAllShaders();
	static ShaderProgram& createShader(const char* vertPath, const char* fragPath);
	static ShaderProgram& createShaderFromSource(const char* vertSource, const char* fragSource);
};