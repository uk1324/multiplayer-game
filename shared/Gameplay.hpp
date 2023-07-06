#pragma once

#include <engine/Math/vec2.hpp>
#include <shared/Networking.hpp>

static constexpr float PLAYER_HITBOX_RADIUS = 0.05f;
static constexpr float BULLET_HITBOX_RADIUS = 0.04f;
//static constexpr float BULLET_SPEED = 0.5f;

static constexpr float PLAYER_SPEED = 1.0f;
static constexpr float PLAYER_SHIFT_SPEED = 0.7f;
static constexpr float BULLET_SPEED = 0.5f;
static constexpr float SHOOT_COOLDOWN = 0.4f;

Vec2 applyMovementInput(Vec2 pos, ClientInputMessage::Input input, float dt);
void updateBullet(Vec2& position, Vec2 velocity, f32& timeElapsed, f32& timeToCatchUp, i32& aliveFramesLeft, f32 dt);

template<typename Callable>
void spawnTripleBullet(Vec2 pos, float rotation, float velocity, Callable spawnBullet /*(Vec2 pos, Vec2 velocity)*/) {
	spawnBullet(pos, velocity * Vec2::oriented(rotation));
	spawnBullet(pos, velocity * Vec2::oriented(rotation - 0.1f));
	spawnBullet(pos, velocity * Vec2::oriented(rotation + 0.1f));
}