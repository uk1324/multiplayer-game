#include <Engine/Engine.hpp>
#include <Engine/Window.hpp>
#include <optional>
#include <charconv>
#include <iostream>
#include <shared/WindowsUtils.hpp>
#include <shared/DebugWindowInfo.hpp>

// https://www.youtube.com/watch?v=CfZtX8Vj4s0 

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
		quit = Engine::run(60);
		Engine::terminate();
	}
}