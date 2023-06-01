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
	struct PredictedTransform {
		Vec2 pos;
		std::vector<PastInput> inputs;
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
	struct Bullet {
		InterpolatedTransform transform;
	};
	std::unordered_map<PlayerIndex, InterpolatedTransform> playerIndexToTransform;
	std::unordered_map<i32, Bullet> bullets;

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