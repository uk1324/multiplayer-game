#pragma once

#include <engine/Math/Vec2.hpp>
#include <shared/Networking.hpp>

static constexpr float PLAYER_HITBOX_RADIUS = 0.05f;
static constexpr float BULLET_HITBOX_RADIUS = 0.04f;
//static constexpr float BULLET_SPEED = 0.5f;

static constexpr float PLAYER_SPEED = 1.0f;
static constexpr float PLAYER_SHIFT_SPEED = 0.7f;
static constexpr float BULLET_SPEED = 0.5f;
static constexpr float SHOOT_COOLDOWN = 0.4f;

Vec2 applyMovementInput(Vec2 pos, ClientInputMessage::Input input, float dt);

void updateGameplayPlayer(GameplayPlayer& player, const ClientInputMessage::Input& input, float dt);
//void updateBullet(Vec2& position, Vec2 velocity, f32& timeElapsed, f32& timeToCatchUp, i32& aliveFramesLeft, f32 dt);
//
//struct GameplayBullet {
//	//enum class Type {
//	//	MOVE_FORWARD,
//	//};
//
//	//Vec2 pos;
//	//Vec2 velocity;
//};
//
//// SpawnBullet = (Vec2 pos, Vec2 velocity) -> int id
//// GetBullet = (int id) -> std::optional<
//
//struct InvertedCircleSpawner {
//	float time;
//
//	template<typename T> 
//	void update() {
//
//	}
//};
//
//template<typename SpawnBullet>
//void spawnTripleBullet(Vec2 pos, float rotation, float velocity, SpawnBullet spawnBullet) {
//	/*for (int i = 0; i < 10; i++) {
//		float angle = i / 10.0f * 6.28f;
//		spawnBullet(pos, velocity * Vec2::oriented(angle));
//	}*/
//	spawnBullet(pos, velocity * Vec2::oriented(rotation));
//	/*spawnBullet(pos, velocity * Vec2::oriented(rotation));
//	spawnBullet(pos, velocity * Vec2::oriented(rotation - 0.1f));
//	spawnBullet(pos, velocity * Vec2::oriented(rotation + 0.1f));*/
//}