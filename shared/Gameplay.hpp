#pragma once

#include <shared/Math/vec2.hpp>
#include <shared/Networking.hpp>

static constexpr float PLAYER_HITBOX_RADIUS = 0.05f;
static constexpr float BULLET_HITBOX_RADIUS = 0.01f;
static constexpr float BULLET_SPEED = 0.5f;

Vec2 applyMovementInput(Vec2 pos, ClientInputMessage::Input input, float dt);
void updateBullet(Vec2& position, Vec2 velocity, i32& framesElapsed, f32& timeToCatchUp, i32& aliveFramesLeft);