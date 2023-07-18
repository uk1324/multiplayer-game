#include <server/ServerGameplayContext.hpp>
#include <server/GameServer.hpp>
#include <engine/Utils/MapOptGet.hpp>

ServerGameplayContext::ServerGameplayContext(GameServer& server)
	: server(server) {}

std::optional<const GameplayPlayer&> ServerGameplayContext::getPlayer(PlayerIndex playerIndex) {
	const auto player = get(server.players, playerIndex);
	if (player.has_value()) {
		return player->gameplayPlayer;
	}
	return std::nullopt;
}
