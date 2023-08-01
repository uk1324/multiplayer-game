#pragma once

#include <yojimbo/yojimbo.h>
#include <GameServerAdapter.hpp>
#include <shared/Networking.hpp>
#include <server/DebugSettings.hpp>
#include <shared/Time.hpp>
#include <shared/Gameplay.hpp>
#include <shared/ReplayRecorder.hpp>
#include <unordered_map>
#include <queue>

struct GameServer {
	GameServer(const char* address);
	~GameServer();
	void update();
	void processMessages();
	void processMessage(int clientIndex, yojimbo::Message* message);
	void onClientConnected(int clientIndex);
	void onClientDisconnected(int clientIndex);

	void broadcastWorldState();

	bool isRunning = true;

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

	// Using this instead of stroing one of the indices inside the player, because its more flexible. Sometimes you onle have the player index (gameplay). or only have the client index (processing messages).
	std::unordered_map<int, PlayerIndex> clientIndexToPlayerIndex;
	PlayerIndex nextPlayerIndex{ 0 };

	yojimbo::Server server;
	GameServerAdapter adapter;

	#ifdef DEBUG_REPLAY_RECORDER
	ReplayRecorder replayRecorder;
	std::vector<GameplayPlayer> gameplayPlayers;
	#endif
};