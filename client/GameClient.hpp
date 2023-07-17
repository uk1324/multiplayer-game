#pragma once

#include <client/Rendering/Renderer.hpp>
#include <optional>
#include <shared/Networking.hpp>
#include <shared/BulletIndex.hpp>
#include <shared/Time.hpp>
#include <shared/ReplayRecorder.hpp>

//#define DEBUG_INTERPOLATE_BULLETS
// Could use static variables and use Imgui to enable or disable it.
//#define DEBUG_INTERPOLATION

// TODO: Maybe for safety always use a function that somehow requires you to check if the entity exists before you can access it. The problem with optional is that you can access it without needing to check.
	// TODO: Use a ring buffer.
struct GameClient {
	GameClient(yojimbo::Client& client, Renderer& renderer);
	void update();

	void processMessage(yojimbo::Message* message);

	void onJoin(const JoinMessage& msg);
	void onDisconnected();
	PlayerIndex clientPlayerIndex = -1;
	bool joinedGame() const;

	FrameTime sequenceNumber = 0;

	FrameTime newestUpdateLastReceivedClientSequenceNumber = 0;
	FrameTime newestUpdateServerSequenceNumber = 0;

	std::vector<FrameTime> pastReceiveDelays;
	std::vector<FrameTime> pastExecuteDelays;
	FrameTime averageReceiveDelay = 0;
	// Could calculate the executed delay by sending how many inputs does the server have buffered up.
	FrameTime averageExecuteDelay = 0;

	struct Delays {
		// serverFrame to clientFrame 
		FrameTime interpolatedEntitesDisplayDelay;
		FrameTime receiveDelay;
		FrameTime executeDelay;
	};
	std::optional<Delays> delays;

	struct InterpolatedTransform {
		struct Position {
			Vec2 position;
			FrameTime serverFrame;
		};

		std::vector<Position> positions;
		Vec2 position;

		void updatePositions(Vec2 newPosition, FrameTime serverFrame);
		void interpolatePosition(FrameTime sequenceNumber, const Delays& delays);

		void debugDisplay() const;
	};

	std::unordered_map<PlayerIndex, InterpolatedTransform> playerIndexToTransform;
	struct PastInput {
		ClientInputMessage::Input input;
		FrameTime sequenceNumber;
	};
	std::vector<PastInput> pastInputs;
	GameplayPlayer clientPlayer;
	GameplayState gameplayState;
	GameplayState inactiveGameplayState;

	struct Player {
		Vec2 position;
		// TODO: Change this to false.
		bool isRendered = true;
	};
	std::unordered_map<PlayerIndex, Player> players;

	#ifdef DEBUG_INTERPOLATE_BULLETS
	std::unordered_map<UntypedBulletIndex, InterpolatedTransform> debugInterpolatedBullets;
	#endif 

	ReplayRecorder replayRecorder = ReplayRecorder("./generated/clientReplay.json");

	yojimbo::Client& client;
	Renderer& renderer;
};