#include <Engine/Engine.hpp>

int main() {
	bool quit = false;
	while (!quit) {
		Engine::init(480, 600, "game");
		quit = Engine::run(60);
		Engine::terminate();
	}
}