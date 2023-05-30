#include <chrono>
#include <GameServer.hpp>
#include <iostream>

int main() {
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

	while (server.isRunning)
	{
		frameStartTime = currentTime();
		frameTime = frameStartTime - lastFrameStartTime;
		lastFrameStartTime = frameStartTime;

		if (frameTime > FRAME_TIME_CAP)
			frameTime = FRAME_TIME_CAP;

		accumulatedTime += frameTime;

		while (accumulatedTime >= updateTime)
		{
			server.update(updateTime);
			accumulatedTime -= updateTime;
		}
	}

	ShutdownYojimbo();

	return EXIT_SUCCESS;
}