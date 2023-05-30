#pragma once

#include <yojimbo/yojimbo.h>
#include <GameServerAdapter.hpp>
#include <shared/Networking.hpp>
#include <unordered_map>

struct GameServer {
	GameServer();
	float dt;
	void update(float dt);
	void processMessages();
	void processMessage(int clientIndex, yojimbo::Message* message);
	void processClientInputMessage(int clientIndex, ClientInputMessage& msg);
	void onClientConnected(int clientIndex);
	void onClientDisconnected(int clientIndex);

	bool isRunning = true;

	int currentFrame = 0;
	std::unordered_map<PlayerIndex, Vec2> playerIndexToTransform;
	std::unordered_map<PlayerIndex, int> playerIndexToLastReceivedInputFrameValue;

	yojimbo::Server server;
	GameServerAdapter adapter;
};