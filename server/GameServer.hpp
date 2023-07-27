#pragma once

#include <yojimbo/yojimbo.h>
#include <GameServerAdapter.hpp>
#include <shared/Networking.hpp>
#include <shared/Time.hpp>
#include <shared/Gameplay.hpp>
#include <shared/ReplayRecorder.hpp>
#include <unordered_map>
#include <queue>

#ifndef FINAL_RELEASE
//#define DEBUG_REPLAY_RECORDER
#endif


struct GameServer {
	GameServer();
	~GameServer();
	void update();
	void processMessages();
	void processMessage(PlayerIndex clientIndex, yojimbo::Message* message);
	void onClientConnected(int clientIndex);
	void onClientDisconnected(int clientIndex);

	void broadcastWorldState();

	bool isRunning = true;

	//FrameTime sequenceNumber;
	FrameTime frame = 0;


	struct Player {
		std::optional<FrameTime> newestReceivedInputClientSequenceNumber;
		std::optional<FrameTime> newestExecutedInputClientSequenceNumber;
		
		struct InputWithSequenceNumber {
			ClientInputMessage::Input input;
			FrameTime clientSequenceNumber = 0;
		};
		std::queue<InputWithSequenceNumber> inputs;

		LeaderboardEntry leaderboard;
		GameplayPlayer gameplayPlayer;
		// TODO: Change to false later.
		bool isAlive = false;
	};

	GameplayState gameplayState;
	std::unordered_map<PlayerIndex, Player> players;

	yojimbo::Server server;
	GameServerAdapter adapter;

	#ifdef DEBUG_REPLAY_RECORDER
	ReplayRecorder replayRecorder;
	std::vector<GameplayPlayer> gameplayPlayers;
	#endif
};