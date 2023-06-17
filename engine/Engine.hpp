#pragma once

#include <Types.hpp>
#include <string_view>
#include <unordered_map>
#include <memory>

class Engine {
public:
	static void init(int windowWidth, int windowHeight, std::string_view windowTitle);
	static bool run(int fps);
	static void terminate();
	static void stop();

private:
	static void initGlfw();
	static void initOpenGl();
	static void initImGui();

	static void glfwErrorCallback(int errorCode, const char* errorMessage);
	static void __stdcall openGlErrorCallback(u32 source, u32 type, u32 id, u32 severity, i32 length, const char* message, const void* userParam);
};