#include <engine/Engine.hpp>
#include <engine/Window.hpp>
#include <optional>
#include <charconv>
#include <iostream>
#include <string.h>
#include <shared/WindowsUtils.hpp>
#include <shared/DebugWindowInfo.hpp>

// https://www.youtube.com/watch?v=CfZtX8Vj4s0 
// Hybird simluation of pixel partices. Move as an actual particle collide as a pixel particle.

#include <chrono>

auto currentTime = []() {
	using namespace std::chrono;
	return duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
};

struct FixedUpdateLoop {
	FixedUpdateLoop(double fps);

	bool running = true;

	double lastFrameStartTime;
	double frameStartTime;
	double frameTime;

	double accumulatedTime = 0.0;
	//const auto fps = 60;
	double updateFrameTime;

	bool isRunning();
};

FixedUpdateLoop::FixedUpdateLoop(double fps) 
	: updateFrameTime(1.0 / fps)
	, lastFrameStartTime(currentTime()) {
}

bool FixedUpdateLoop::isRunning() {
	constexpr double FRAME_TIME_CAP = 2.0;

	while (accumulatedTime < updateFrameTime) {
		frameStartTime = currentTime();
		frameTime = frameStartTime - lastFrameStartTime;
		lastFrameStartTime = frameStartTime;

		if (frameTime > FRAME_TIME_CAP)
			frameTime = FRAME_TIME_CAP;

		accumulatedTime += frameTime;
	}
	accumulatedTime -= updateFrameTime;
	return running;
}


int main(int argc, char* argv[]) {

	/*FixedUpdateLoop fixedUpdatedLoop(1.0);

	while (fixedUpdatedLoop.isRunning()) {
		std::cout << "test ";
	}*/

	#ifndef FINAL_RELEASE
	std::optional<int> windowIndex;
	if (argc >= 2) {
		int value;
		auto result = std::from_chars(argv[1], argv[1] + strlen(argv[1]) + 1, value);
		if (result.ec == std::errc()) {
			windowIndex = value;
		}
	}
	#endif

	bool quit = false;
	while (!quit) {
		Engine::init(WINDOW_WIDTH, WINDOW_HEIGHT, "game");
		#ifndef FINAL_RELEASE
		if (windowIndex.has_value()) {
			std::cout << *windowIndex << '\n';
			const auto y = 30;
			const auto x = *windowIndex * WINDOW_WIDTH;
			Window::setPos(Vec2(x, y));
			setConsolePosAndSize(x, y + 30 + WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT);
		}
		#endif
		//Window::enableWindowedFullscreen();
		quit = Engine::run(60);
		Engine::terminate();
	}
}

#ifdef FINAL_RELEASE

#ifdef WIN32
#include <Windows.h>
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	return main(0, nullptr);
}
#endif

#endif
