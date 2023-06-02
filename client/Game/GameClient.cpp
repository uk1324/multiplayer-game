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
	client.SetLatency(DEBUG_LATENCY);
	client.SetJitter(DEBUG_JITTER);
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
		/*const auto cursorPos = Input::cursorPos();
		ImGui::Text("%g %g", cursorPos.x, cursorPos.y);*/
		if (client.IsConnected()) {
			processMessages();
			
			if (joinedGame) {
				const auto cursorPos = renderer.camera.screenSpaceToCameraSpace(Input::cursorPos());
				const auto cursorRelativeToPlayer = cursorPos - playerTransform.pos;
				const auto rotation = atan2(cursorRelativeToPlayer.y, cursorRelativeToPlayer.x);
				const auto newInput = ClientInputMessage::Input{
					.up = Input::isKeyHeld(KeyCode::W),
					.down = Input::isKeyHeld(KeyCode::S),
					.left = Input::isKeyHeld(KeyCode::A),
					.right = Input::isKeyHeld(KeyCode::D),
					.shoot = Input::isMouseButtonDown(MouseButton::LEFT),
					.rotation = rotation
				};
				pastInputCommands.push_back(newInput);

				const auto oldCommandsToDiscardCount = static_cast<int>(pastInputCommands.size()) - ClientInputMessage::INPUTS_COUNT;
				if (oldCommandsToDiscardCount > 0) {
					pastInputCommands.erase(pastInputCommands.begin(), pastInputCommands.begin() + oldCommandsToDiscardCount);
				}
				//std::cout << pastInputCommands.size() << '\n';
				
				const auto inputMsg = static_cast<ClientInputMessage*>(client.CreateMessage(static_cast<int>(GameMessageType::CLIENT_INPUT)));

				inputMsg->sequenceNumber = sequenceNumber;

				int offset = ClientInputMessage::INPUTS_COUNT - static_cast<int>(pastInputCommands.size());
				for (int i = 0; i < pastInputCommands.size(); i++) {
					inputMsg->inputs[offset + i] = pastInputCommands[i];
				}

				//std::cout << "sending " << inputMsg->sequenceNumber << '\n';
				client.SendMessage(GameChannel::UNRELIABLE, inputMsg);

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

	for (auto& [_, transform] : playerIndexToTransform) {
		transform.updateInterpolatedPosition(sequenceNumber);
	}
	for (auto& [_, bullet] : bullets) {
		bullet.transform.updateInterpolatedPosition(sequenceNumber);
		bullet.aliveFramesLeft--;
	}

	for (auto it = bullets.begin(); it != bullets.end();) {
		if (it->second.aliveFramesLeft >= 0)
			++it;
		else
			it = bullets.erase(it);
	}

	for (auto& [_, transform] : playerIndexToTransform) {
		renderer.drawSprite(renderer.bulletSprite, transform.pos, PLAYER_HITBOX_RADIUS * 2.0f);
	}
	for (auto& [_, bullet] : bullets) {
		const auto opacityChangeFrames = 60.0f;
		const auto opacity = 1.0f - std::clamp((opacityChangeFrames - bullet.aliveFramesLeft) / opacityChangeFrames, 0.0f, 1.0f);
		renderer.drawSprite(renderer.bulletSprite, bullet.transform.pos, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(1.0f, 1.0f, 1.0f, opacity));
	}
	/*renderer.drawSprite(renderer.bullet2Sprite, playerTransform.pos, PLAYER_HITBOX_RADIUS * 2.0f);*/
	renderer.drawSprite(renderer.bulletSprite, playerTransform.pos, PLAYER_HITBOX_RADIUS * 2.0f);
	//renderer.drawSprite(renderer.bullet3Sprite, playerTransform.pos, PLAYER_HITBOX_RADIUS * 2.0f);
	renderer.update(playerTransform.pos);
	sequenceNumber++;
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
			}
			break;
		}

		case GameMessageType::CLIENT_INPUT:
			ASSERT_NOT_REACHED();
			break;

		case GameMessageType::WORLD_UPDATE: { 
			const auto msg = static_cast<WorldUpdateMessage*>(message);
			if (msg->sequenceNumber < newestUpdateSequenceNumber) {
				std::cout << "out of order update message";
				break;
			}

			const auto playersSize = msg->playersCount * sizeof(WorldUpdateMessage::Player);
			const auto bulletsSize = msg->bulletsCount * sizeof(WorldUpdateMessage::Bullet);
			const auto dataSize = playersSize + bulletsSize;

			if (msg->GetBlockSize() != dataSize) {
				ASSERT_NOT_REACHED();
				break;
			}
			newestUpdateSequenceNumber = msg->sequenceNumber;
			newestUpdateLastReceivedClientSequenceNumber = msg->lastReceivedClientSequenceNumber;

			const auto msgPlayers = reinterpret_cast<WorldUpdateMessage::Player*>(msg->GetBlockData());
			const auto msgBullets = reinterpret_cast<WorldUpdateMessage::Bullet*>(msg->GetBlockData() + playersSize);

			for (int i = 0; i < msg->playersCount; i++) {
				const auto& player = msgPlayers[i];
				if (player.index == clientPlayerIndex) {
					playerTransform.pos = player.position;
					
					std::erase_if(
						playerTransform.inputs,
						[&](const PastInput& prediction) {
							return prediction.sequenceNumber <= newestUpdateLastReceivedClientSequenceNumber;
						}
					);

					// Maybe store the positions when the prediction is made and the compare the predicted ones with the server ones and only rollback the old state if they don't match. 
					// https://youtu.be/zrIY0eIyqmI?t=1599
					for (const auto& prediction : playerTransform.inputs) {
						playerTransform.pos = applyMovementInput(playerTransform.pos, prediction.input, dt);
					}
				} else {
					auto& transform = playerIndexToTransform[player.index];
					//transform.pos = player.position;
					transform.positions.push_back(InterpolationPosition{ 
						.pos = player.position, 
						.frameToDisplayAt = sequenceNumber + SERVER_UPDATE_SEND_RATE_DIVISOR 
					});
				}
			}

			for (int i = 0; i < msg->bulletsCount; i++) {
				const auto& msgBullet = msgBullets[i];
				auto& bullet = bullets[msgBullet.index];
				bullet.transform.positions.push_back(InterpolationPosition{
					.pos = msgBullet.position,
					.frameToDisplayAt = sequenceNumber + SERVER_UPDATE_SEND_RATE_DIVISOR,
				});
				bullet.aliveFramesLeft = msgBullet.aliveFramesLeft;
				bullet.ownerPlayerId = msgBullet.ownerPlayerId;
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
				auto t = static_cast<float>(sequenceNumber - positions[i].frameToDisplayAt) / 6.0f;
				t = std::clamp(t, 0.0f, 1.0f);

				const auto start = positions[i].pos;
				const auto end = positions[i + 1].pos;
				pos = lerp(start, end, t);
				// Can't use hermite interpolation because it overshoots, which makes it look like it's rubber banding.
				// TODO: The overhsooting might not happen if I store more frames, but this would also add more latency. But I don't think that would actually fix that.
				// https://gdcvault.com/play/1024597/Replicating-Chaos-Vehicle-Replication-in
			}
		}

		if (positions.size() > 2) {
			positions.erase(positions.begin(), positions.begin() + i - 1);
		}
	}
}
