#include "Gameplay.hpp"

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
	pos += direction.normalized() * dt;
	return pos;
}

void updateBullet(Vec2& position, Vec2 velocity, f32& timeElapsed, f32& timeToCatchUp, i32& aliveFramesLeft) {
	float speedUp = 0.0f;

	if (timeToCatchUp > 0.0f) {
		const auto catchUpSpeed = 0.08f;
		speedUp = (timeToCatchUp * catchUpSpeed);
		timeToCatchUp -= speedUp;

		if (timeToCatchUp <= FRAME_DT / 2.0f) {
			speedUp += timeToCatchUp;
			timeToCatchUp = 0.0f;
		}
	}

	const auto elapsed = FRAME_DT + speedUp;
	position += velocity * elapsed;
	aliveFramesLeft--;
	timeElapsed += elapsed;
}
