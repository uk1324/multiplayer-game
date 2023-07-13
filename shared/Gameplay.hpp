#pragma once

#include <engine/Math/Vec2.hpp>
#include <shared/Networking.hpp>

// TODO: It would be cool to have a debugger for patterns that would allow have a slider for time.
// The simplest way to do this would to on a change of time just to reset to the initial state and update which should be enought.
// A more complicated version would to to store the previous states or maybe just the delays (this would work if using things like hot reloading).
// Technically you could make a language that could calculate the inverses of operations. (How would that work if things like position rely on velocity and vice versa).

static constexpr float PLAYER_HITBOX_RADIUS = 0.05f;
static constexpr float BULLET_HITBOX_RADIUS = 0.04f;
//static constexpr float BULLET_SPEED = 0.5f;

static constexpr float PLAYER_SPEED = 1.0f;
static constexpr float PLAYER_SHIFT_SPEED = 0.7f;
static constexpr float BULLET_SPEED = 0.5f;
static constexpr float SHOOT_COOLDOWN = 0.4f;

Vec2 applyMovementInput(Vec2 pos, ClientInputMessage::Input input, float dt);

// The update is separated like this, so when they do an input they see the state the are reacting to when the input is executed.
void updateGemeplayStateBeforeProcessingInput(GameplayState& state);
void updateGameplayPlayer(
	PlayerIndex playerIndex,
	GameplayPlayer& player,
	GameplayState& state,
	const ClientInputMessage::Input& input,
	FrameTime ownerFrame,
	float dt);
void updateGemeplayStateAfterProcessingInput(GameplayState& state, float dt);