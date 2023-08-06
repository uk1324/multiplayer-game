#include <chrono>
#include <GameServer.hpp>
#include <iostream>
#include <shared/WindowsUtils.hpp>
#include <shared/DebugWindowInfo.hpp>
#include <server/DebugSettings.hpp>
#include <charconv>
#include <server/GetAddress.hpp>

#ifdef DEBUG_DRAW
#include <replayTool/debugDraw.hpp>
#include <raylib/raylib.h>
#include <shared/Gameplay.hpp>

void onClose() {
	system("taskkill /IM client.exe");
}
#endif

int main(int argc, char* argv[]) {
	#ifdef DEBUG_DRAW
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

	Camera2D camera{
		.offset = Vector2{ 0.0f, 0.0f },
		.target = 0.0f,
		.rotation = 0.0f,
		.zoom = 500.0f,
		//.zoom = 1.0f,
	};
	//float scale = 500.0f;
	camera.offset.x += windowWidth / 2;
	camera.offset.y += windowHeight / 2;
	std::vector<GameplayPlayer> players;

	#endif
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

	char defaultAddress[] = "127.0.0.1";
	char address[MAX_ADDRESS_SIZE];
	
	#ifdef FINAL_RELEASE
	if (argc == 2) {
		const auto inputAddress = argv[1];
		const auto inputAddressLength = strlen(inputAddress);
		// '>=' because of the '\0' byte.
		if (inputAddressLength >= sizeof(address)) {
			std::cerr << "address too long\n";
			return EXIT_FAILURE;
		}
		memcpy(address, inputAddress, inputAddressLength + 1);
	} else if (!getAddress(address, sizeof(address))) {
		std::cerr << "getAddress failed\n";
		memcpy(address, defaultAddress, sizeof(defaultAddress));
	}
	#else
	memcpy(address, defaultAddress, sizeof(defaultAddress));
	#endif

	GameServer server(address);

	auto isRunning = [&]() -> bool {
		#ifdef DEBUG_DRAW
		return server.isRunning && !WindowShouldClose();
		#else
		return server.isRunning;
		#endif
	};

	// TODO: Does raylib slow down the game loop, because of things like vsync.
	while (isRunning())
	{
		frameStartTime = currentTime();
		frameTime = frameStartTime - lastFrameStartTime;
		lastFrameStartTime = frameStartTime;

		if (frameTime > FRAME_TIME_CAP)
			frameTime = FRAME_TIME_CAP;

		accumulatedTime += frameTime;

		while (accumulatedTime >= updateTime) {
			server.update();
			accumulatedTime -= updateTime;

			#ifdef DEBUG_DRAW
			BeginDrawing();
			ClearBackground(BLACK);

			BeginMode2D(camera);

			players.clear();
			for (const auto& player : server.players) {
				players.push_back(player.second.gameplayPlayer);
			}
			debugDraw(server.gameplayState, players, BLUE);

			EndMode2D();
			EndDrawing();
			#endif
		}
	}
	#ifdef DEBUG_DRAW
	CloseWindow();
	#endif

	// Raylib causes yojimbo to leak memory for some reason. If this is commented out the leak check doesn't happen.
	// ShutdownYojimbo();

	return EXIT_SUCCESS;
}