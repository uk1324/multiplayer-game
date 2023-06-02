#pragma once

#include <Game/Renderer.hpp>
#include <Game/GameClientAdapter.hpp>

class GameClient 
{
public:
	GameClient();
	~GameClient();
	float dt;
	void update(float dt);
	void processMessages();
	void processMessage(yojimbo::Message* message);

	struct PastInput {
		ClientInputMessage::Input input;
		int sequenceNumber;
	};
	struct PredictedPlayerTransform {
		Vec2 pos;
		std::vector<PastInput> inputs;
	};
	PredictedPlayerTransform playerTransform;

	struct InterpolationPosition {
		Vec2 pos;
		int frameToDisplayAt;
	};

	struct InterpolatedTransform {
		std::vector<InterpolationPosition> positions;
		Vec2 pos;

		void updateInterpolatedPosition(int currentFrame);
	};

	struct PredictedTrasform {
		struct PredictedTranslation {
			Vec2 translation;
			int sequenceNumber;
		};

		std::vector<PredictedTranslation> predictedTranslations;
		Vec2 pos;

		void setAuthoritativePosition(Vec2 newPos, int sequenceNumber);
	};

	struct InterpolatedBullet {
		InterpolatedTransform transform;
		i32 ownerPlayerIndex = -1;
		i32 aliveFramesLeft = -1;
		i32 startDisplayingAtFrame = 0;
	};

	// Make the bullet slower at the start. Calculate the velocity so the prediction and the actuall one meet at (rtt + someTime) time. Then switch the bullets at that time. This gives an advantage to the players with lower rtt, because their bullets will match the predicted bullets more closely. They won't have to lead their shots. The predicted bullets would be able to go throught players, because they aren't the actually positions of the players and this might confuse players. Another issue is making the velocitites continuous (and not positions not to overshoot doing so). The current implementation isn't even C0 continuous most of the time lol.
	struct PredictedBullet {
		/*PredictedTrasform transform;*/
		float elapsed;
		Vec2 pos;
		Vec2 velocity;
		int spawnSequenceNumber;
		int frameSpawnIndex;
		int displayDelay;
		int destroyAt = 0;
	};
	std::vector<PredictedBullet> predictedBullets;

	int thisFrameSpawnIndexCounter = 0;

	std::unordered_map<PlayerIndex, InterpolatedTransform> playerIndexToTransform;
	std::unordered_map<i32, InterpolatedBullet> interpolatedBullets;
	//std::unordered_map<i32, PredictedBullet> predictedBullets;

	std::vector<ClientInputMessage::Input> pastInputCommands;

	int newestUpdateLastReceivedClientSequenceNumber = 0;
	int newestUpdateSequenceNumber = 0;

	i32 sequenceNumber = 0;
	bool joinedGame = false;
	PlayerIndex clientPlayerIndex = -1;

	yojimbo::Client client;
	GameClientAdapter adapter;

	Renderer renderer;
};