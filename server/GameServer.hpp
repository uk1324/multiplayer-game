#pragma once

#include <yojimbo/yojimbo.h>
#include <GameServerAdapter.hpp>
#include <shared/Networking.hpp>
#include <unordered_map>
#include <queue>

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

	int sequenceNumber = 0;

	struct Player {
		Vec2 pos{ 0.0f };
		i32 newestReceivedInputSequenceNumber = 0;
		i32 newestExecutedInputSequenceNumber = 0;
		bool receivedInputThisFrame = false; // Predict input based on last input?
		// Executing multiple inputs in a frame to compensate for missed inputs frames would allow the user to cheat by sending intentionally not sending messages and then sending a message with the old inputs, which would allow them to teleport (could maybe interpolate between state to mitigate it) and to know the future before executing the inputs (if the ping is low enough, smaller than the time that it has to wait between the sending of hacked messages).
		i32 framesWithoutInputReceived = 0;
		struct InputWithSequenceNumber {
			ClientInputMessage::Input input;
			int sequenceNumber = 0;
		};
		std::queue<InputWithSequenceNumber> inputs;
	};
	std::unordered_map<PlayerIndex, Player> players;


	yojimbo::Server server;
	GameServerAdapter adapter;
};