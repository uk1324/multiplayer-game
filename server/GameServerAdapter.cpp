#include <GameServerAdapter.hpp>
#include <GameServer.hpp>
#include <shared/Networking.hpp>

GameServerAdapter::GameServerAdapter(GameServer* server)
	: server(server) {}

yojimbo::MessageFactory* GameServerAdapter::CreateMessageFactory(yojimbo::Allocator& allocator) {
	return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
}

void GameServerAdapter::OnServerClientConnected(int clientIndex) {
	if (server != nullptr) {
		server->onClientConnected(clientIndex);
	}
}

void GameServerAdapter::OnServerClientDisconnected(int clientIndex) {
	if (server != nullptr) {
		server->onClientDisconnected(clientIndex);
	}
}