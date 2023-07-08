#include <chrono>
#include <GameServer.hpp>
#include <iostream>
#include <shared/WindowsUtils.hpp>
#include <shared/DebugWindowInfo.hpp>
#include <charconv>

#include <raylib/raylib.h>
#include <shared/Gameplay.hpp>

void onClose() {
	system("taskkill /IM client.exe");
}

int main(int argc, char* argv[]) {
	std::optional<int> openedClientWindows;
	if (argc >= 2) {
		int value;
		auto result = std::from_chars(argv[1], argv[1] + strlen(argv[1]) + 1, value);
		if (result.ec == std::errc()) {
			openedClientWindows = value;
		}
	}
	if (openedClientWindows.has_value()) {
		int screenWidth, screenHeight;
		getPrimaryScreenSize(&screenWidth, &screenHeight);
		const auto x = WINDOW_WIDTH * *openedClientWindows;
		const auto sizeX = screenWidth - x;
		const auto sizeY = WINDOW_HEIGHT;

		InitWindow(sizeX, sizeY, "server");
		SetWindowPosition(x, 30);

		setConsolePosAndSize(x, 30 + WINDOW_HEIGHT + 30, sizeX, sizeY);
	} else {
		InitWindow(800, 600, "server");
	}
	setOnCloseCallback(onClose);

	const auto windowWidth = GetScreenWidth();
	const auto windowHeight = GetScreenHeight();


	constexpr double FRAME_TIME_CAP = 2.0;
	auto currentTime = []() {
		using namespace std::chrono;
		return duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
	};
	double lastFrameStartTime = currentTime();
	double frameStartTime;
	double frameTime;

	double accumulatedTime = 0.0;
	const auto fps = 60;
	const double updateTime = 1.0 / fps;
	if (!InitializeYojimbo()) {
		std::cout << "failed to initialize Yojimbo";
		return EXIT_FAILURE;
	}
	yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

	GameServer server;

	auto vector2 = [](Vec2 pos) {
		return Vector2{ .x = pos.x, .y = pos.y };
	};

	auto convertPos = [&](Vec2 pos) {
		pos.y = -pos.y;
		return vector2(pos);
	};

	Camera2D camera{
		.offset = Vector2{ 0.0f, 0.0f },
		.target = 0.0f,
		.rotation = 0.0f,
		//.zoom = 500.0f,
		.zoom = 1.0f,
	};
	float scale = 500.0f;
	camera.offset.x += windowWidth / 2;
	camera.offset.y += windowHeight / 2;

	while (server.isRunning && !WindowShouldClose())
	{
		frameStartTime = currentTime();
		frameTime = frameStartTime - lastFrameStartTime;
		lastFrameStartTime = frameStartTime;

		if (frameTime > FRAME_TIME_CAP)
			frameTime = FRAME_TIME_CAP;

		accumulatedTime += frameTime;

		while (accumulatedTime >= updateTime) {
			for (const auto& [_, bullet] : server.bullets) {
				std::cout << bullet.timeElapsed << '\n';
			}

			server.update(updateTime);
			accumulatedTime -= updateTime;

			BeginDrawing();
			ClearBackground(BLACK);

			BeginMode2D(camera);

			if (server.players.contains(0)) {
				//camera.offset = convertPos(server.players[0].pos * scale);
				camera.target = convertPos(server.players[0].pos * scale);
				//std::cout << camera.offset.x << ' ' << camera.offset.y << '\n';
			}
			for (const auto& [_, player] : server.players) {
				DrawCircleV(convertPos(player.pos * scale), PLAYER_HITBOX_RADIUS * scale, BLUE);
			}

			for (const auto& [_, bullet] : server.bullets) {
				DrawCircleV(convertPos(bullet.pos * scale), BULLET_HITBOX_RADIUS * scale, BLUE);
			}

			EndMode2D();
			EndDrawing();
		}
	}
	CloseWindow();

	// Raylib causes yojimbo to leak memory for some reason. If this is commented out the leak check doesn't happen.
	//ShutdownYojimbo();

	return EXIT_SUCCESS;
}