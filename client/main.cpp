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

int main(int argc, char* argv[]) {
	std::optional<int> windowIndex;
	if (argc >= 2) {
		int value;
		auto result = std::from_chars(argv[1], argv[1] + strlen(argv[1]) + 1, value);
		if (result.ec == std::errc()) {
			windowIndex = value;
		}
	}

	bool quit = false;
	while (!quit) {
		Engine::init(WINDOW_WIDTH, WINDOW_HEIGHT, "game");
		if (windowIndex.has_value()) {
			std::cout << *windowIndex << '\n';
			const auto y = 30;
			const auto x = *windowIndex * WINDOW_WIDTH;
			Window::setPos(Vec2(x, y));
			setConsolePosAndSize(x, y + 30 + WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT);
		}
		Window::enableWindowedFullscreen();
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