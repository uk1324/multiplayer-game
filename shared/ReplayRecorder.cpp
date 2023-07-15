#include "ReplayRecorder.hpp"
#include <fstream>
#include <iostream>
#include <engine/Json/JsonPrinter.hpp>

using namespace std::chrono;

ReplayRecorder::ReplayRecorder(std::string outputFile)
	: outputFile(std::move(outputFile))
	, start(Clock::now()) {
	start = sys_days{ July / 15 / 2023 } + 0h + 0min + 0s;
}

ReplayRecorder::~ReplayRecorder() {
	std::ofstream file(outputFile);
	Json::print(file, toJson(replay));
	std::cout << "saved replay";
}

void ReplayRecorder::addFrame(std::vector<GameplayPlayer> players, GameplayState gameplayState) {
	replay.frames.push_back(ReplayFrame{
		.globalClockTime = duration<float>(Clock::now() - start).count(),
		.players = std::move(players),
		.gameplayState = std::move(gameplayState),
	});
}
