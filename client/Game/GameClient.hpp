#pragma once

#include <Game/Renderer.hpp>
#include <Game/GameClientAdapter.hpp>

class GameClient 
{
public:
	GameClient();
	~GameClient();

	void update(float dt);
	void processMessages();
	void processMessage(yojimbo::Message* message);

	struct PredictedTranslation {
		Vec2 translation;
		int frame;
	};
	struct PredictedTransform {
		Vec2 pos;
		std::vector<PredictedTranslation> predictedTranslations;
	};
	PredictedTransform playerTransform;

	struct InterpolationPosition {
		Vec2 pos;
		int frameToDisplayAt;
	};
	struct InterpolatedTransform {
		std::vector<InterpolationPosition> positions;
		Vec2 pos;

		void updateInterpolatedPosition(int currentFrame);
	};

	std::unordered_map<PlayerIndex, InterpolatedTransform> playerIndexToTransform;

	int lastReceivedUpdateFrame = 0;

	int currentFrame = 0;
	bool joinedGame = false;
	PlayerIndex clientPlayerIndex;

	

	yojimbo::Client client;
	GameClientAdapter adapter;

	Renderer renderer;
};