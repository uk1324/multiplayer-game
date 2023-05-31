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
