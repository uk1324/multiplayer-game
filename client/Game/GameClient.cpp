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
// Speed up and the slow down again parabolic?
GameClient::~GameClient() {
	client.Disconnect();
}

#include <imgui.h>
#include <iostream>

// accelerate bullets on everyones, but the shooters side.
// decelerate the bullets at the start on the client. (which just means they accelerate)
// 

void GameClient::update(float dt) {
	/*yojimbo::NetworkInfo info;
	client.GetNetworkInfo(info);
	{
		ImGui::Text("RTT %g", info.RTT);
		ImGui::Text("packetLoss %g", info.packetLoss);
		ImGui::Text("sentBandwidth %g", info.sentBandwidth);
		ImGui::Text("receivedBandwidth %g", info.receivedBandwidth);
		ImGui::Text("ackedBandwidth %g", info.ackedBandwidth);
		ImGui::Text("numPacketsSent %d", info.numPacketsSent);
		ImGui::Text("numPacketsReceived %d", info.numPacketsReceived);
		ImGui::Text("numPacketsAcked %d", info.numPacketsAcked);
	}*/
	ImGui::TextWrapped("blue bullets are spawned when the message first message with them is received and later predicted red bullets are the positions interpolated between server updates");
	this->dt = dt;
	thisFrameSpawnIndexCounter = 0;
	{
		// update client
		client.AdvanceTime(client.GetTime() + dt);
		client.ReceivePackets();
		if (client.IsConnected()) {
			processMessages();
			
			if (joinedGame) {
				const auto cursorPos = renderer.camera.screenSpaceToCameraSpace(Input::cursorPos());
				const auto cursorRelativeToPlayer = cursorPos - playerTransform.position;
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

				const auto newPos = applyMovementInput(playerTransform.position, newInput, dt);
				playerTransform.inputs.push_back(PastInput{
					.input = newInput,
					.sequenceNumber = sequenceNumber
				});
				playerTransform.position = newPos;

				/*if (newInput.shoot) {
					const auto direction = Vec2::oriented(newInput.rotation);
					predictedBullets.push_back(PredictedBullet{
						.elapsed = 0.0f,
						.position = playerTransform.position + PLAYER_HITBOX_RADIUS * direction,
						.velocity = direction * BULLET_SPEED,
						.spawnSequenceNumber = sequenceNumber,
						.frameSpawnIndex = thisFrameSpawnIndexCounter,
					});
					thisFrameSpawnIndexCounter++;
				}*/
			}
		}

		client.SendPackets();
	}

	for (auto& [_, transform] : playerIndexToTransform) {
		transform.interpolatePosition(sequenceNumber);
	}

	for (auto& [_, bullet] : interpolatedBullets) {
		bullet.transform.interpolatePosition(sequenceNumber);
		bullet.aliveFramesLeft--;
	}
	std::erase_if(interpolatedBullets, [](const auto& item) { return item.second.aliveFramesLeft <= 0; });

	for (auto& bullet : predictedBullets) {
		if (sequenceNumber <= bullet.frameToActivateAt)
			continue;

		renderer.drawSprite(renderer.bulletSprite, bullet.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(0.5f, 1.0f, 1.0f));
		updateBullet(bullet.position, bullet.velocity, bullet.framesElapsed, bullet.timeToCatchUp, bullet.aliveFramesLeft);
	}

	for (auto& [_, transform] : playerIndexToTransform) {
		renderer.drawSprite(renderer.bulletSprite, transform.position, PLAYER_HITBOX_RADIUS * 2.0f);
	}

	auto calculateBulletOpacity = [](int aliveFramesLeft) {
		const auto opacityChangeFrames = 60.0f;
		const auto opacity = 1.0f - std::clamp((opacityChangeFrames - aliveFramesLeft) / opacityChangeFrames, 0.0f, 1.0f);
		return opacity;
	};

	for (auto& [_, bullet] : interpolatedBullets) {
		const auto opacity = calculateBulletOpacity(bullet.aliveFramesLeft);
		renderer.drawSprite(renderer.bulletSprite, bullet.transform.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(1.0f, 1.0f, 1.0f, opacity));
	}

	renderer.drawSprite(renderer.bulletSprite, playerTransform.position, PLAYER_HITBOX_RADIUS * 2.0f);
	renderer.update(playerTransform.position);
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
			// when recieving update for frame x then check if the predicted bullets actually got spawned and delete them if not.
			const auto msg = static_cast<WorldUpdateMessage*>(message);
			if (msg->sequenceNumber < newestUpdateSequenceNumber) {
				std::cout << "out of order update message";
				break;
			}

			if (!firstWorldUpdate.has_value()) {
				firstWorldUpdate = FirstUpdate{
					.serverSequenceNumber = msg->sequenceNumber,
					.sequenceNumber = sequenceNumber,
				};
			}
			const auto& firstUpdate = *firstWorldUpdate;

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
				const auto& msgPlayer = msgPlayers[i];
				if (msgPlayer.index == clientPlayerIndex) {
					playerTransform.position = msgPlayer.position;
					
					std::erase_if(
						playerTransform.inputs,
						[&](const PastInput& prediction) {
							return prediction.sequenceNumber <= newestUpdateLastReceivedClientSequenceNumber;
						}
					);

					// Maybe store the positions when the prediction is made and the compare the predicted ones with the server ones and only rollback the old state if they don't match. 
					// https://youtu.be/zrIY0eIyqmI?t=1599
					for (const auto& prediction : playerTransform.inputs) {
						playerTransform.position = applyMovementInput(playerTransform.position, prediction.input, dt);
					}
				} else {
					auto& transform = playerIndexToTransform[msgPlayer.index];
					transform.updatePositions(firstUpdate, msgPlayer.position, sequenceNumber, msg->sequenceNumber);
				}
			}

			for (int i = 0; i < msg->bulletsCount; i++) {
				const auto& msgBullet = msgBullets[i];

				auto& bullet = interpolatedBullets[msgBullet.index];
				const auto spawn = bullet.transform.positions.size() == 0;
				bullet.transform.updatePositions(firstUpdate, msgBullet.position, sequenceNumber, msg->sequenceNumber);
				if (spawn) {
					const auto& p = bullet.transform.positions.back();
					
					predictedBullets.push_back(PredictedBullet{
						.position = p.position,
						.velocity = msgBullet.velocity,
						// The prediction also has to be delayed to be in synch with the players the client sees
						.frameToActivateAt = p.frameToDisplayAt,
						.framesElapsed = msgBullet.framesElapsed,
						.timeToCatchUp = msgBullet.timeToCatchUp,
						.aliveFramesLeft = msgBullet.aliveFramesLeft,
					});
					// If the bullet should have already been activated forward the prediction in time.
					auto& bullet = predictedBullets.back();
					while (bullet.frameToActivateAt < sequenceNumber) { // Should this be < or <= ?
						updateBullet(bullet.position, bullet.velocity, bullet.framesElapsed, bullet.timeToCatchUp, bullet.aliveFramesLeft);
						bullet.frameToActivateAt++;
					}

				}
				bullet.aliveFramesLeft = msgBullet.aliveFramesLeft;
				bullet.ownerPlayerIndex = msgBullet.ownerPlayerIndex;
			}

			break;
		}

		case GameMessageType::TEST:
			std::cout << "hit\n";
			break;
	}
}

void GameClient::InterpolatedTransform::updatePositions(const FirstUpdate& firstUpdate, Vec2 newPosition, int sequenceNumber, int serverSequenceNumber) {
	const auto displayDelay = 6;
	positions.push_back(InterpolationPosition{
		.position = newPosition,
		.frameToDisplayAt = 
			firstUpdate.sequenceNumber +  
			(serverSequenceNumber - firstUpdate.serverSequenceNumber) * SERVER_UPDATE_SEND_RATE_DIVISOR + 
			displayDelay,
		.serverSequenceNumber = serverSequenceNumber
	});
}

void GameClient::InterpolatedTransform::interpolatePosition(int sequenceNumber) {
	std::sort(
		positions.begin(),
		positions.end(),
		[](const InterpolationPosition& a, const InterpolationPosition& b) {
			return a.frameToDisplayAt < b.frameToDisplayAt;
		}
	);
	if (positions.size() == 1) {
		position = positions[0].position;
	} else {
		int i = 0;
		for (i = 0; i < positions.size() - 1; i++) {
			if (positions[i].frameToDisplayAt <= sequenceNumber && positions[i + 1].frameToDisplayAt > sequenceNumber) {
				auto t = 
					static_cast<float>(sequenceNumber - positions[i].frameToDisplayAt) / 
					static_cast<float>(positions[i + 1].frameToDisplayAt - positions[i].frameToDisplayAt);
				t = std::clamp(t, 0.0f, 1.0f);
				if (t == 1.0) {
					std::cout << "lost";
				}

				const auto start = positions[i].position;
				const auto end = positions[i + 1].position;
				position = lerp(start, end, t);
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

void GameClient::PredictedTrasform::setAuthoritativePosition(Vec2 newPos, int sequenceNumber) {
	std::erase_if(
		predictedTranslations,
		[&](const PredictedTranslation& prediction) {
			return prediction.sequenceNumber <= sequenceNumber;
		}
	);

	position = newPos;
	for (const auto& prediction : predictedTranslations) {
		position += prediction.translation;
	}
}
