#pragma once

#include <stdint.h>

class Time
{
public:
	static float deltaTime();
	static uint64_t currentTick();

	static float currentTime();

	static void update(float deltaTime);

private:
	static double _deltaTime;
	static double _lastFrameTime;	
	static uint64_t _currentTick;
};