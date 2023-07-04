#pragma once

#include <engine/Math/Vec2.hpp>
#include <string_view>

class Window {
public:
	static void init(int width, int height, std::string_view title);
	static void terminate();

	static Vec2 size();
	static float aspectRatio();

	// https://www.glfw.org/docs/3.0/group__clipboard.html
	// "The returned string is valid only until the next call to glfwGetClipboardString or glfwSetClipboardString."
	static const char* getClipboard();
	static void setClipboard(const char* string);

	static void update();
	static void close();

	static void setTitle(const char* title);

	static void setPos(Vec2 pos);

	static void enableWindowedFullscreen();

	static void hideCursor();
	static void showCursor();

	static bool resized();

	static bool shouldClose();

	static void* handle();
};