#pragma once

#include <shared/GameplayStateData.hpp>
#include <engine/Utils/RefOptional.hpp>

struct GameplayContext {
	virtual std::optional<const GameplayPlayer&> getPlayer(PlayerIndex playerIndex) = 0;
};