#pragma once

#include <shared/GameplayStateData.hpp>
#include <raylib/raylib.h>

void debugDraw(const GameplayState& gameplayState, const std::vector<GameplayPlayer>& players, Color color);