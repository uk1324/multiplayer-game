#pragma once

#include <shared/Math/vec2.hpp>
#include <shared/Networking.hpp>

Vec2 applyMovementInput(Vec2 pos, ClientInputMessage::Input input, float dt);