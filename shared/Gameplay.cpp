#include "Gameplay.hpp"

template<typename BulletIndex>
static BulletIndex createBulletIndex(GameplayState& gameplayState, FrameTime ownerFrame, PlayerIndex ownerPlayerIndex) {
	const auto spawnIndex = gameplayState.bulletsSpawnedThisFrame;
	gameplayState.bulletsSpawnedThisFrame++;
	return BulletIndex{
		.ownerPlayerIndex = ownerPlayerIndex,
		.spawnFrame = ownerFrame,
		.spawnIndexInFrame = static_cast<u16>(spawnIndex),
	};
}

Vec2 applyMovementInput(Vec2 pos, ClientInputMessage::Input input, float dt) {
	Vec2 direction(0.0f);
	if (input.down) {
		direction += Vec2(0.0f, -1.0f);
	}
	if (input.up) {
		direction += Vec2(0.0f, 1.0f);
	}
	if (input.left) {
		direction += Vec2(-1.0f, 0.0f);
	}
	if (input.right) {
		direction += Vec2(1.0f, 0.0f);
	}
	const auto velocity = input.shift ? PLAYER_SHIFT_SPEED : PLAYER_SPEED;
	pos += direction.normalized() * dt * velocity;
	return pos;
}

void updateGemeplayStateBeforeProcessingInput(GameplayState& state) {
	state.bulletsSpawnedThisFrame = 0;
}

void updateGameplayPlayer(
	PlayerIndex playerIndex, 
	GameplayPlayer& player, 
	GameplayState& state, 
	const ClientInputMessage::Input& input, 
	FrameTime ownerFrame,
	float dt) {
	player.position = applyMovementInput(player.position, input, dt);


	const auto direction = Vec2::oriented(input.rotation);

	player.shootCooldown = std::max(0.0f, player.shootCooldown - dt);
	if (input.shoot && player.shootCooldown == 0.0f) {
		player.shootCooldown = SHOOT_COOLDOWN;
		const auto index = createBulletIndex<MoveForwardBulletIndex>(state, ownerFrame, playerIndex);
		state.moveForwardBullets[index] = MoveForwardBullet{
			.position = player.position,
			.velocity = direction * 0.5f
		};
	}
}

void updateGemeplayStateAfterProcessingInput(GameplayState& state, float dt) {
	for (auto& [index, bullet] : state.moveForwardBullets) {
		bullet.position += bullet.velocity * dt;
	}
}

void updateBullet(Vec2& position, Vec2 velocity, f32& timeElapsed, f32& timeToCatchUp, i32& aliveFramesLeft, f32 dt) {
	float speedUp = 0.0f;

	if (timeToCatchUp > 0.0f) {
		const auto catchUpSpeed = 0.08f;
		speedUp = (timeToCatchUp * catchUpSpeed);
		timeToCatchUp -= speedUp;

		if (timeToCatchUp <= dt / 2.0f) {
			speedUp += timeToCatchUp;
			timeToCatchUp = 0.0f;
		}
	}

	const auto elapsed = dt + speedUp;
	position += velocity * elapsed;
	aliveFramesLeft--;
	timeElapsed += elapsed;
}
