#pragma once

#include <client/Rendering/Renderer.hpp>
#include <optional>
#include <shared/Networking.hpp>

// TODO: Maybe for safety always use a function that somehow requires you to check if the entity exists before you can access it. The problem with optional is that you can access it without needing to check.
class GameClient 
{
public:
	GameClient(yojimbo::Client& client, Renderer& renderer);
	~GameClient();
	void update();

	void processMessage(yojimbo::Message* message);
	bool receivedWorldUpdateThisFrame = false;

	static double time();

	double joinTime;
	void onJoin(int playerIndex);
	bool joinedGame();
	void disconnect();

	float elapsed = 0.0f;

	struct FirstUpdate {
		int serverSequenceNumber;
		int sequenceNumber;
	};
	std::optional<FirstUpdate> firstWorldUpdate;

	struct PastInput {
		ClientInputMessage::Input input;
		int sequenceNumber;
	};
	struct PredictedPlayerTransform {
		Vec2 position;
		std::vector<PastInput> inputs;
	};

	struct InterpolationPosition {
		Vec2 position;
		int frameToDisplayAt;
		int serverSequenceNumber;
	};

	struct InterpolatedTransform {
		std::vector<InterpolationPosition> positions;
		Vec2 position;

		void updatePositions(const FirstUpdate& firstUpdate, Vec2 newPosition, int sequenceNumber, int serverSequenceNumber);
		void interpolatePosition(int currentFrame);
	};

	struct PredictedTrasform {
		struct PredictedTranslation {
			Vec2 translation;
			int sequenceNumber;
		};

		std::vector<PredictedTranslation> predictedTranslations;
		Vec2 position;

		void setAuthoritativePosition(Vec2 newPos, int sequenceNumber);
	};

	struct InterpolatedBullet {
		InterpolatedTransform transform;
		i32 ownerPlayerIndex = -1;
		i32 aliveFramesLeft = -1;
	};

	struct PredictedBullet {
		Vec2 position;
		Vec2 velocity;
		int spawnSequenceNumber;
		int frameSpawnIndex;
		int frameToActivateAt;
		float timeElapsed; 
		float timeToCatchUp;
		int aliveFramesLeft;
		float timeToSynchornize;
		float tSynchronizaztion = 0.0f;
		int testLink = -1;
	};

	struct LeaderboardEntry {
		int kills = 0;
		int deaths = 0;
	};

	PredictedPlayerTransform playerTransform;
	float shootCooldown = 0.0f;
	bool isAlive = true;

	struct Player {
		LeaderboardEntry leaderboard;
		bool isRendered = true;
		Vec2 position;
	};

	Vec3 getPlayerColor(int id);

	std::unordered_map<int, Player> players;
	//std::unordered_map<i32, LeaderboardEntry> playerIdToLeaderboardEntry;


	std::vector<PredictedBullet> predictedBullets;

	int thisFrameSpawnIndexCounter = 0;

	std::unordered_map<i32, InterpolatedTransform> playerIndexToTransform;
	std::unordered_map<i32, InterpolatedBullet> interpolatedBullets;
	//std::unordered_map<i32, PredictedBullet> predictedBullets;

	std::vector<ClientInputMessage::Input> pastInputCommands;

	int newestUpdateLastReceivedClientSequenceNumber = 0;
	int newestUpdateSequenceNumber = -1;

	i32 sequenceNumber = 0;

	i32 clientPlayerIndex = -1;

	yojimbo::Client& client;

	Renderer& renderer;
};