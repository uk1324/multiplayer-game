#pragma once

#include <yojimbo/yojimbo.h>
#include <GameServerAdapter.hpp>
#include <shared/Networking.hpp>
#include <shared/Time.hpp>
#include <unordered_map>
#include <queue>

struct GameServer {
	GameServer();
	~GameServer();
	void update();
	void processMessages();
	void processMessage(PlayerIndex clientIndex, yojimbo::Message* message);
	void onClientConnected(int clientIndex);
	void onClientDisconnected(int clientIndex);

	bool isRunning = true;

	//FrameTime sequenceNumber;
	FrameTime frame = 0;


	struct Player {
		FrameTime newestReceivedInputClientSequenceNumber = 0;
		FrameTime newestExecutedInputClientSequenceNumber = 0;
		
		struct InputWithSequenceNumber {
			ClientInputMessage::Input input;
			FrameTime clientSequenceNumber = 0;
		};
		std::queue<InputWithSequenceNumber> inputs;
	};

	std::unordered_map<PlayerIndex, Player> players;

	yojimbo::Server server;
	GameServerAdapter adapter;
};