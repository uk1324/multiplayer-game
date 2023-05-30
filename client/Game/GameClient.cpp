#include <Game/GameClient.hpp>
#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <Engine/Engine.hpp>
#include <shared/Math/utils.hpp>
#include <shared/Networking.hpp>
#include <shared/Utils/Types.hpp>
#include <shared/Gameplay.hpp>

GameClient::GameClient()
	: client(yojimbo::GetDefaultAllocator(), yojimbo::Address("0.0.0.0"), connectionConfig, adapter, 0.0) {
	u64 clientId;
	yojimbo::random_bytes((uint8_t*)&clientId, 8);
	client.InsecureConnect(DEFAULT_PRIVATE_KEY, clientId, yojimbo::Address("127.0.0.1", SERVER_PORT));
	client.SetLatency(1000.0f);
	//client.SetPacketLoss(0.1f);
	//playerIdToTransform[clientId] = Transform{ .pos = Vec2{ 0.0f } };
}

GameClient::~GameClient() {
	client.Disconnect();
}

#include <imgui.h>
#include <iostream>

void GameClient::update(float dt) {

	{
		// update client
		client.AdvanceTime(client.GetTime() + dt);
		client.ReceivePackets();

		if (client.IsConnected()) {
			processMessages();
			
			if (joinedGame) {
				const auto inputMsg = static_cast<ClientInputMessage*>(client.CreateMessage(static_cast<int>(GameMessageType::CLIENT_INPUT)));
				inputMsg->up = Input::isKeyHeld(KeyCode::W);
				inputMsg->left = Input::isKeyHeld(KeyCode::A);
				inputMsg->down = Input::isKeyHeld(KeyCode::S);
				inputMsg->right = Input::isKeyHeld(KeyCode::D);
				inputMsg->frame = currentFrame;
				std::cout << "sent " << currentFrame << '\n';
				client.SendMessage(GameChannel::UNRELIABLE, inputMsg);

				
				const auto newPos = applyMovementInput(playerTransform.pos, inputMsg->up, inputMsg->down, inputMsg->left, inputMsg->right, dt);
				playerTransform.predictedTranslations.push_back(PredictedTranslation{
					.translation = newPos - playerTransform.pos,
					.frame = currentFrame
				});
				playerTransform.pos = newPos;
			}
		}

		client.SendPackets();
	}

	for (auto& [_, transform] : playerIndexToTransform) {
		transform.updateInterpolatedPosition(currentFrame);
	}

	for (auto& [id, transform] : playerIndexToTransform) {
		const auto& positions = transform.positions;
		//Vec2 pos;
		//if (positions.size() == 1) {
		//	pos = positions[0].pos;
		//} else {
		//	int i = 0;
		//	for (i = 0; i < positions.size() - 1; i++) {
		//		if (positions[i].frameToDisplayAt <= currentFrame && positions[i + 1].frameToDisplayAt > currentFrame) {
		//			const auto t = static_cast<float>(currentFrame - positions[i].frameToDisplayAt) / 6.0f;
		//			if (i == 0) {
		//				const auto start = positions[i].pos;
		//				const auto end = positions[i + 1].pos;
		//				pos = lerp(start, end, t);
		//			} else {
		//				const auto start = positions[i].pos;
		//				const auto startVel = (start - positions[i - 1].pos) / (6 * 0.1f);
		//				const auto end = positions[i + 1].pos;
		//				auto endVel = (end - start) / (6 * 0.1f);
		//				if (i + 1 < positions.size()) {
		//					endVel = (positions[i + 1].pos - end) / (6 * 0.1f);
		//				}
		//				pos = cubicHermite(start, startVel, end, endVel, t);
		//			}
		//			
		//		}
		//	}

		//	if (positions.size() > 3) {
		//		/*transform.positions.erase(transform.positions.begin(), transform.positions.begin() + i - 1);*/
		//		transform.positions.erase(transform.positions.begin(), transform.positions.begin() + i - 2);
		//	}
		//}
		renderer.drawSprite(renderer.bulletSprite, transform.pos, 0.1f);
	}
	renderer.drawSprite(renderer.bullet2Sprite, playerTransform.pos, 0.1f);
	currentFrame++;
	renderer.update();
}

void GameClient::processMessages() {
	for (int i = 0; i < connectionConfig.numChannels; i++) {
		yojimbo::Message* message = client.ReceiveMessage(i);
		while (message != nullptr) {
			processMessage(message);
			client.ReleaseMessage(message);
			message = client.ReceiveMessage(i);
		}
	}
}

void GameClient::processMessage(yojimbo::Message* message) {
	switch (message->GetType()) {
		case GameMessageType::JOIN: {
			if (joinedGame) {
				ASSERT_NOT_REACHED();
			} else {
				const auto msg = static_cast<JoinMessage*>(message);
				clientPlayerIndex = msg->clientPlayerIndex;
				std::cout << "client joined clientPlayerIndex = " << clientPlayerIndex << '\n';
				joinedGame = true;
				//currentFrame = msg->currentFrame;
			}
			break;
		}

		case GameMessageType::CLIENT_INPUT:
			ASSERT_NOT_REACHED();
			break;

		case GameMessageType::PLAYER_POSITION_UPDATE: { 
			const auto msg = static_cast<PlayerPositionUpdateMessage*>(message);
			/*if (msg->lastReceivedInputFrameValue < lastReceivedUpdateFrame) {
				std::cout << "out of order update message\n";
				break;
			}*/
			lastReceivedUpdateFrame = msg->lastReceivedInputFrameValue;
			const auto positionCount = msg->GetBlockSize() / sizeof(PlayerPosition);
			if (msg->GetBlockSize() % sizeof(PlayerPosition) != 0) {
				ASSERT_NOT_REACHED();
				break;
			}
			const auto positions = reinterpret_cast<PlayerPosition*>(msg->GetBlockData());

			std::cout << "received " << msg->lastReceivedInputFrameValue << '\n';
			for (int i = 0; i < positionCount; i++) {
				const auto& position = positions[i];
				if (position.playerIndex == clientPlayerIndex) {
					playerTransform.pos = position.position;
					std::erase_if(
						playerTransform.predictedTranslations,
						[&](const PredictedTranslation& prediction) { return prediction.frame < lastReceivedUpdateFrame; }
					);

					for (const auto& prediction : playerTransform.predictedTranslations) {
						playerTransform.pos += prediction.translation;
					}
				} else {
					auto& transform = playerIndexToTransform[position.playerIndex];
					transform.positions.push_back(InterpolationPosition{ .pos = position.position, .frameToDisplayAt = currentFrame + 6 });
				}
			}
			break;
		}
	}
}

void GameClient::InterpolatedTransform::updateInterpolatedPosition(int currentFrame) {
	if (positions.size() == 1) {
		pos = positions[0].pos;
	} else {
		int i = 0;
		for (i = 0; i < positions.size() - 1; i++) {
			if (positions[i].frameToDisplayAt <= currentFrame && positions[i + 1].frameToDisplayAt > currentFrame) {
				const auto t = static_cast<float>(currentFrame - positions[i].frameToDisplayAt) / 6.0f;
				if (i == 0) {
					const auto start = positions[i].pos;
					const auto end = positions[i + 1].pos;
					pos = lerp(start, end, t);
				} else {
					const auto start = positions[i].pos;
					const auto startVel = (start - positions[i - 1].pos) / (6 * 0.1f);
					const auto end = positions[i + 1].pos;
					auto endVel = (end - start) / (6 * 0.1f);
					if (i + 1 < positions.size()) {
						endVel = (positions[i + 1].pos - end) / (6 * 0.1f);
					}
					pos = cubicHermite(start, startVel, end, endVel, t);
				}

			}
		}

		if (positions.size() > 3) {
			/*transform.positions.erase(transform.positions.begin(), transform.positions.begin() + i - 1);*/
			positions.erase(positions.begin(), positions.begin() + i - 2);
		}
	}
}
