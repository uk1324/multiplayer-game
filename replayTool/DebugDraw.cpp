#include "debugDraw.hpp"
#include <shared/Gameplay.hpp>

void debugDraw(const GameplayState& gameplayState, const std::vector<GameplayPlayer>& players, Color color) {
	auto vector2 = [](Vec2 pos) {
		return Vector2{ .x = pos.x, .y = pos.y };
	};

	auto convertPos = [&](Vec2 pos) {
		pos.y = -pos.y;
		return vector2(pos);
	};

	for (const auto& player : players) {
		DrawCircleV(convertPos(player.position), PLAYER_HITBOX_RADIUS, color);
	}

	for (const auto& [_, bullet] : gameplayState.moveForwardBullets) {
		DrawCircleV(convertPos(bullet.position), BULLET_HITBOX_RADIUS, color);
	}
}
