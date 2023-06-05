#pragma once

#include <Game/Renderer.hpp>
#include <Game/GameClientAdapter.hpp>
#include <optional>

class GameClient 
{
public:
	GameClient();
	~GameClient();
	float dt;
	void update(float dt);
	void processMessages();
	void processMessage(yojimbo::Message* message);

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
	PredictedPlayerTransform playerTransform;

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
		int framesElapsed; 
		float timeToCatchUp;
		int aliveFramesLeft;
	};
	std::vector<PredictedBullet> predictedBullets;

	int thisFrameSpawnIndexCounter = 0;

	std::unordered_map<PlayerIndex, InterpolatedTransform> playerIndexToTransform;
	std::unordered_map<i32, InterpolatedBullet> interpolatedBullets;
	//std::unordered_map<i32, PredictedBullet> predictedBullets;

	std::vector<ClientInputMessage::Input> pastInputCommands;

	int newestUpdateLastReceivedClientSequenceNumber = 0;
	int newestUpdateSequenceNumber = -1;

	i32 sequenceNumber = 0;
	bool joinedGame = false;
	PlayerIndex clientPlayerIndex = -1;

	yojimbo::Client client;
	GameClientAdapter adapter;

	Renderer renderer;
};