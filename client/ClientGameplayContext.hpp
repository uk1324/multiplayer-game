#include <shared/GameplayContext.hpp>

struct GameClient;

struct ClientGameplayContext : public GameplayContext {
	ClientGameplayContext(GameClient& client);

	std::optional<const GameplayPlayer&> getPlayer(PlayerIndex playerIndex) override;
	GameClient& client;
};