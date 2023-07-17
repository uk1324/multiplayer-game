#pragma once

#include <shared/ReplayData.hpp>
#include <chrono>

struct ReplayRecorder {
	ReplayRecorder(std::string outputFile);
	~ReplayRecorder();

	using Clock = std::chrono::system_clock;

	void addFrame(std::vector<GameplayPlayer> players, GameplayState gameplayState);

	std::chrono::time_point<Clock> start;
	bool isRecording = false;
	ReplayData replay;
	std::string outputFile;
};