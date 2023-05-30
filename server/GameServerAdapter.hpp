#pragma once

#include <yojimbo/yojimbo.h>

class GameServer;

class GameServerAdapter : public yojimbo::Adapter {
public:
	explicit GameServerAdapter(GameServer* server);
	yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override;
	void OnServerClientConnected(int clientIndex) override;
	void OnServerClientDisconnected(int clientIndex) override;

private:
	GameServer* server;
};