#include <client/Rendering/ShaderManager.hpp>
#include <filesystem>
#include <iostream>

struct ShaderEntry {
	std::string_view vertPath;
	std::filesystem::file_time_type vertPathLastWriteTime;
	std::string_view fragPath;
	std::filesystem::file_time_type fragPathLastWriteTime;
	ShaderProgram program;

	void tryReload() {
		auto result = ShaderProgram::compile(vertPath, fragPath);
		if (const auto error = std::get_if<ShaderProgram::Error>(&result)) {
			std::cout << "tried to reload vert: " << vertPath << " frag: " << fragPath << '\n';
			std::cout << error->toSingleMessage();
		} else {
			std::cout << "reloaded shader\n"
				<< "vert: " << vertPath << '\n'
				<< "frag: " << fragPath << '\n';
			program = std::move(std::get<ShaderProgram>(result));
		}
	}
};

// Using list so pointers aren't invalidated.
std::list<ShaderEntry> shaderEntries;

void ShaderManager::terminate() {
	shaderEntries.clear();
}

void reloadChangedShaders() {
	for (auto& shader : shaderEntries) {
		try {
			const auto vertLastWriteTime = std::filesystem::last_write_time(shader.vertPath);
			const auto fragLastWriteTime = std::filesystem::last_write_time(shader.fragPath);
			if (shader.vertPathLastWriteTime == vertLastWriteTime && shader.fragPathLastWriteTime == fragLastWriteTime) {
				continue;
			}
			shader.tryReload();
			shader.vertPathLastWriteTime = vertLastWriteTime;
			shader.fragPathLastWriteTime = fragLastWriteTime;
		} catch (std::filesystem::filesystem_error) {

		}
	}
}

void ShaderManager::update() {
	reloadChangedShaders();
}

void ShaderManager::reloadAllShaders() {
	for (auto& shader : shaderEntries) {
		shader.tryReload();
	}
}

ShaderProgram& ShaderManager::createShader(const char* vertPath, const char* fragPath) {
	shaderEntries.push_back(ShaderEntry{
		.vertPath = vertPath,
		.fragPath = fragPath,
		.program = ShaderProgram::create(vertPath, fragPath)
	});
	auto& shader = shaderEntries.back();
	shader.vertPathLastWriteTime = std::filesystem::last_write_time(shader.vertPath);
	shader.fragPathLastWriteTime = std::filesystem::last_write_time(shader.fragPath);
	return shader.program;
}
