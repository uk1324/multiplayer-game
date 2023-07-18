#pragma once

#include <shared/GameplayContext.hpp>

struct GameServer;

struct ServerGameplayContext : public GameplayContext {
	ServerGameplayContext(GameServer& server);

	std::optional<const GameplayPlayer&> getPlayer(PlayerIndex playerIndex) override;
	GameServer& server;
};