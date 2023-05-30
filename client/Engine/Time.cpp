#include <Engine/Time.hpp>

#include <GLFW/glfw3.h>

float Time::deltaTime()
{
	return static_cast<float>(_deltaTime);
}

uint64_t Time::currentTick()
{
	return _currentTick;
}

float Time::currentTime()
{
	return static_cast<float>(glfwGetTime());
}

void Time::update(float deltaTime)
{
	// Don't know if I should use the actual delta time or should it be a constant so every program execution is the same.
	double currentTime = glfwGetTime();
	//m_deltaTime = currentTime - m_lastFrameTime;
	_deltaTime = deltaTime;
	_lastFrameTime = currentTime;
	_currentTick++;
}

double Time::_deltaTime = 0.0;
double Time::_lastFrameTime = 0.0;
uint64_t Time::_currentTick = 0;
