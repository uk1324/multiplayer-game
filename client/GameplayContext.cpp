#include <client/ClientGameplayContext.hpp>
#include <client/GameClient.hpp>

ClientGameplayContext::ClientGameplayContext(GameClient& client) 
	: client(client) {}

std::optional<const GameplayPlayer&> ClientGameplayContext::getPlayer(PlayerIndex playerIndex) {
	if (client.clientPlayerIndex == playerIndex) {
		return client.clientPlayer;
	}
	return std::nullopt;
}