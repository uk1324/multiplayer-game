#pragma once

#include <engine/Math/Vec2.hpp>
#include <string_view>

class Window {
public:
	static void init(int width, int height, std::string_view title);
	static void terminate();

	static Vec2 size();
	static float aspectRatio();

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