#include "Gameplay.hpp"

Vec2 applyMovementInput(Vec2 pos, bool up, bool down, bool left, bool right, float dt) {
	Vec2 direction(0.0f);
	if (down) {
		direction += Vec2(0.0f, -1.0f);
	}
	if (up) {
		direction += Vec2(0.0f, 1.0f);
	}
	if (left) {
		direction += Vec2(-1.0f, 0.0f);
	}
	if (right) {
		direction += Vec2(1.0f, 0.0f);
	}
	pos += direction.normalized() * dt;
	return pos;
}
