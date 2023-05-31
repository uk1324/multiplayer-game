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
	client.SetJitter(50.0f);
	/*client.SetLatency(1000.0f);
	client.SetJitter(200.0f);*/
	//client.SetPacketLoss(0.1f);
	//playerIdToTransform[clientId] = Transform{ .pos = Vec2{ 0.0f } };
}

GameClient::~GameClient() {
	client.Disconnect();
}

#include <imgui.h>
#include <iostream>

void GameClient::update(float dt) {
	this->dt = dt;
	{
		// update client
		client.AdvanceTime(client.GetTime() + dt);
		client.ReceivePackets();
		
		if (client.IsConnected()) {
			processMessages();
			
			if (joinedGame) {
				const auto newInput = ClientInputMessage::Input{ 
					.up = Input::isKeyHeld(KeyCode::W),
					.down = Input::isKeyHeld(KeyCode::S),
					.left = Input::isKeyHeld(KeyCode::A),
					.right = Input::isKeyHeld(KeyCode::D),
				};
				pastInputCommands.push_back(newInput);

				const auto oldCommandsToDiscardCount = static_cast<int>(pastInputCommands.size()) - ClientInputMessage::INPUTS_COUNT;
				if (oldCommandsToDiscardCount > 0) {
					pastInputCommands.erase(pastInputCommands.begin(), pastInputCommands.begin() + oldCommandsToDiscardCount);
				}
				std::cout << pastInputCommands.size() << '\n';
				
				const auto inputMsg = static_cast<ClientInputMessage*>(client.CreateMessage(static_cast<int>(GameMessageType::CLIENT_INPUT)));

				inputMsg->sequenceNumber = sequenceNumber;

				int offset = ClientInputMessage::INPUTS_COUNT - static_cast<int>(pastInputCommands.size());
				for (int i = 0; i < pastInputCommands.size(); i++) {
					inputMsg->inputs[offset + i] = pastInputCommands[i];
				}

				std::cout << "sending " << inputMsg->sequenceNumber << '\n';
				client.SendMessage(GameChannel::UNRELIABLE, inputMsg);

				/*playerTransform.predictedTranslations.push_back(PredictedTranslation{
					.translation = newPos - playerTransform.pos,
					.sequenceNumber = sequenceNumber
				});*/
				const auto newPos = applyMovementInput(playerTransform.pos, newInput, dt);
				playerTransform.inputs.push_back(PastInput{
					.input = newInput,
					.sequenceNumber = sequenceNumber
				});
				playerTransform.pos = newPos;
			}
		}

		client.SendPackets();
	}

	sequenceNumber++;
	for (auto& [_, transform] : playerIndexToTransform) {
		transform.updateInterpolatedPosition(sequenceNumber);
	}

	for (auto& [id, transform] : playerIndexToTransform) {
		const auto& positions = transform.positions;
		renderer.drawSprite(renderer.bulletSprite, transform.pos, 0.1f);
	}
	renderer.drawSprite(renderer.bullet2Sprite, playerTransform.pos, 0.1f);
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
				sequenceNumber = 0;
				//currentFrame = msg->currentFrame;
			}
			break;
		}

		case GameMessageType::CLIENT_INPUT:
			ASSERT_NOT_REACHED();
			break;

		case GameMessageType::PLAYER_POSITION_UPDATE: { 
			const auto msg = static_cast<PlayerPositionUpdateMessage*>(message);
			//std::cout << "received " << msg->lastReceivedClientSequenceNumber << '\n';
			if (msg->sequenceNumber < newestUpdateSequenceNumber) {
				std::cout << "out of order update message";
					/*<< newestUpdateLastReceivedClientSequenceNumber 
					<< " msg: " << msg->lastReceivedClientSequenceNumber << "\n";*/
				break;
			}
			newestUpdateSequenceNumber = msg->sequenceNumber;
			newestUpdateLastReceivedClientSequenceNumber = msg->lastReceivedClientSequenceNumber;

			const auto positionCount = msg->GetBlockSize() / sizeof(PlayerPosition);
			if (msg->GetBlockSize() % sizeof(PlayerPosition) != 0) {
				ASSERT_NOT_REACHED();
				break;
			}
			const auto positions = reinterpret_cast<PlayerPosition*>(msg->GetBlockData());

			for (int i = 0; i < positionCount; i++) {
				const auto& position = positions[i];
				if (position.playerIndex == clientPlayerIndex) {
					playerTransform.pos = position.position;
					/*std::erase_if(
						playerTransform.predictedTranslations,
						[&](const PredictedTranslation& prediction) { 
							return prediction.sequenceNumber <= newestUpdateLastReceivedClientSequenceNumber; 
						}
					);*/
					std::erase_if(
						playerTransform.inputs,
						[&](const PastInput& prediction) {
							return prediction.sequenceNumber <= newestUpdateLastReceivedClientSequenceNumber;
						}
					);

					// Maybe store the positions when the prediction is made and the compare the predicted ones with the server ones and only rollback the old state if they don't match. 
					// https://youtu.be/zrIY0eIyqmI?t=1599
					for (const auto& prediction : playerTransform.inputs) {
						//playerTransform.pos += prediction.translation;
						playerTransform.pos = applyMovementInput(playerTransform.pos, prediction.input, dt);
					}
				} else {
					auto& transform = playerIndexToTransform[position.playerIndex];
					transform.positions.push_back(InterpolationPosition{ .pos = position.position, .frameToDisplayAt = sequenceNumber + 6 });
				}
			}
			break;
		}
	}
}

void GameClient::InterpolatedTransform::updateInterpolatedPosition(int sequenceNumber) {
	if (positions.size() == 1) {
		pos = positions[0].pos;
	} else {
		int i = 0;
		for (i = 0; i < positions.size() - 1; i++) {
			if (positions[i].frameToDisplayAt <= sequenceNumber && positions[i + 1].frameToDisplayAt > sequenceNumber) {
				const auto t = static_cast<float>(sequenceNumber - positions[i].frameToDisplayAt) / 6.0f;
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
