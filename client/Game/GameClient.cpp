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

void display(const yojimbo::NetworkInfo& info) {
	ImGui::Text("RTT %g", info.RTT);
	ImGui::Text("packetLoss %g", info.packetLoss);
	ImGui::Text("sentBandwidth %g", info.sentBandwidth);
	ImGui::Text("receivedBandwidth %g", info.receivedBandwidth);
	ImGui::Text("ackedBandwidth %g", info.ackedBandwidth);
	ImGui::Text("numPacketsSent %d", info.numPacketsSent);
	ImGui::Text("numPacketsReceived %d", info.numPacketsReceived);
	ImGui::Text("numPacketsAcked %d", info.numPacketsAcked);
}

// accelerate bullets on everyones, but the shooters side.
// decelerate the bullets at the start on the client. (which just means they accelerate)
// 

void GameClient::update(float dt) {
	//ImGui::Text("The bullets created by the client are predict spawned and later slowed down to make them synced up with the players. So when you hit someone on your screen it should also hit the player on the server. The bullets created by the other players are accelerated to compensate for RTT time, because the player receives the update RTT/2 time after it happened and it takes RTT/2 time for the input to arrive on the server. The client tries to predict the future so they when the dodge the bullets on their screen the bullets should also be dodged on the server later. So the player's bullets and other player's positions are in the past, but other player's bullets are in the future");
	
	this->dt = dt;
	thisFrameSpawnIndexCounter = 0;
	{
		// update client
		client.AdvanceTime(client.GetTime() + dt);
		client.ReceivePackets();
		if (client.IsConnected()) {
			processMessages();
			
			if (joinedGame && !isAlive) {
				if (Input::isKeyDown(KeyCode::SPACE)) {
					const auto message = client.CreateMessage(GameMessageType::SPAWN_REQUEST);
					client.SendMessage(GameChannel::RELIABLE, message);
				}
			}

			if (joinedGame && isAlive) {
				const auto cursorPos = renderer.camera.screenSpaceToCameraSpace(Input::cursorPos());
				const auto cursorRelativeToPlayer = cursorPos - playerTransform.position;
				const auto rotation = atan2(cursorRelativeToPlayer.y, cursorRelativeToPlayer.x);
				const auto newInput = ClientInputMessage::Input{
					.up = Input::isKeyHeld(KeyCode::W),
					.down = Input::isKeyHeld(KeyCode::S),
					.left = Input::isKeyHeld(KeyCode::A),
					.right = Input::isKeyHeld(KeyCode::D),
					/*.shoot = Input::isMouseButtonDown(MouseButton::LEFT),*/
					.shoot = Input::isMouseButtonHeld(MouseButton::LEFT),
					.shift = Input::isKeyHeld(KeyCode::LEFT_SHIFT),
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

				shootCooldown -= dt;
				shootCooldown = std::max(0.0f, shootCooldown);
				if (newInput.shoot && shootCooldown == 0.0f) {
					shootCooldown = SHOOT_COOLDOWN;
					auto spawnBullet = [this](Vec2 pos, Vec2 velocity) {
						const auto direction = velocity.normalized();
						predictedBullets.push_back(PredictedBullet{
							.position = pos + PLAYER_HITBOX_RADIUS * direction,
							.velocity = velocity,
							.spawnSequenceNumber = sequenceNumber,
							.frameSpawnIndex = thisFrameSpawnIndexCounter,
							.frameToActivateAt = -1,
							.timeToCatchUp = 0.0f,
							.timeToSynchornize = 0.0f,
							.tSynchronizaztion = 1.0f
						});
						thisFrameSpawnIndexCounter++;
					};
					spawnTripleBullet(playerTransform.position, newInput.rotation, BULLET_SPEED, spawnBullet);
				}
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

		/*if (bullet.timeToSynchornize) {
			renderer.drawSprite(renderer.bulletSprite, bullet.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(0.5f, 1.0f, 1.0f));
		} else {
		}*/
		renderer.drawSprite(renderer.bulletSprite, bullet.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(1.0f, 1.0f, 1.0f));
		/*if (bullet.timeToSynchornize) {
			renderer.drawSprite(renderer.bulletSprite, bullet.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(0.5f, 1.0f, 1.0f));
		} else {
			renderer.drawSprite(renderer.bulletSprite, bullet.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(1.0f, 1.0f, 1.0f));
		}*/
		auto dt = FRAME_DT;
		if (bullet.timeToSynchornize < 0.0f) {
			bullet.timeToSynchornize = -bullet.timeToSynchornize;
		}

		// integration test.
		const auto n = 100;
		auto s = 1.0f / n;
		float in = 0.0f;
		float max = 1.5;
		const auto scale = 4.0f / max;
		s *= scale;
		for (int i = 0; i < n; i++) {
			float t = i / static_cast<float>(n);
			t -= 0.5f;
			t *= scale;
			in += max * exp(-PI<float> * pow(max * t, 2.0f)) * s;
		}

		//static f32 n = 0.0f;
		static float sum = 0.0f;
		static float maximum;
		ImGui::Text("expected max: %g", FRAME_DT / 8.0f);
		ImGui::Text("calculated max: %g", maximum);
		if (bullet.tSynchronizaztion < 1.0f) {

			auto x = (bullet.tSynchronizaztion - 0.5f); // from -0.5 to 0.5
			// Most of the values are concentrated in the interval -2 to 2
			const auto scale = 4.0f;
			x *= scale; // from -2 to 2

			const auto normalDistributionMax = 1.0f / sqrt(PI<float>);
			const auto value = exp(-pow(x, 2.0f)) / sqrt(PI<float>);

			const auto max = FRAME_DT / 8.0f;
			// max = normalDistMax * functionStep * bullet.timeToSynchronize
			// functionStep = max / (normalDistMax * bullet.timeToSynchronize)
			// functionStep = timeStep * scale
			// timeStep = functionStep / scale
			const auto timeStep = max / (normalDistributionMax * bullet.timeToSynchornize) / scale;

			bullet.tSynchronizaztion += timeStep;
			const auto functionStep = timeStep * scale;
			sum += value * functionStep;
			dt -= value * functionStep * bullet.timeToSynchornize;

			auto val = value * functionStep * bullet.timeToSynchornize;
			if (val > maximum) {
				maximum = val;
			}
		}

		updateBullet(bullet.position, bullet.velocity, bullet.timeElapsed, bullet.timeToCatchUp, bullet.aliveFramesLeft, dt);
	}

	for (auto& [_, transform] : playerIndexToTransform) {
		renderer.drawSprite(renderer.bulletSprite, transform.position, PLAYER_HITBOX_RADIUS * 2.0f);
	}
	if (isAlive) {
		renderer.drawSprite(renderer.bulletSprite, playerTransform.position, PLAYER_HITBOX_RADIUS * 2.0f);
	}

	/*playerTransform.position = Vec2(0.0f);
	if (Input::isKeyDown(KeyCode::L)) {
		renderer.playDeathAnimation(Vec2(0.0f));
	}*/

	auto calculateBulletOpacity = [](int aliveFramesLeft) {
		const auto opacityChangeFrames = 60.0f;
		const auto opacity = 1.0f - std::clamp((opacityChangeFrames - aliveFramesLeft) / opacityChangeFrames, 0.0f, 1.0f);
		return opacity;
	};

	//for (auto& [_, bullet] : interpolatedBullets) {
	//	const auto opacity = calculateBulletOpacity(bullet.aliveFramesLeft);
	//	renderer.drawSprite(renderer.bulletSprite, bullet.transform.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(1.0f, 1.0f, 1.0f, opacity));
	//}

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
						.frameToActivateAt = p.frameToDisplayAt - 6,
						.timeElapsed = msgBullet.timeElapsed,
						.timeToCatchUp = msgBullet.timeToCatchUp,
						.aliveFramesLeft = msgBullet.aliveFramesLeft,
						.timeToSynchornize = 0.0f,
						.tSynchronizaztion = 1.0f
					});
					// If the bullet should have already been activated forward the prediction in time.
					auto& bullet = predictedBullets.back();
					while (bullet.frameToActivateAt < sequenceNumber) { // Should this be < or <= ?
						updateBullet(bullet.position, bullet.velocity, bullet.timeElapsed, bullet.timeToCatchUp, bullet.aliveFramesLeft, FRAME_DT);
						bullet.frameToActivateAt++;
					}
					yojimbo::NetworkInfo info;
					client.GetNetworkInfo(info);
					//bullet.position += bullet.velocity * (info.RTT / 1000.0f);
					bullet.timeToCatchUp += (info.RTT / 1000.0f);
					for (auto& spawnPredictedBullet : predictedBullets) {
						if (spawnPredictedBullet.spawnSequenceNumber == msgBullet.spawnFrameClientSequenceNumber 
							&& spawnPredictedBullet.frameSpawnIndex == msgBullet.frameSpawnIndex) {
							bullet.frameToActivateAt += 6;
							bullet.timeToCatchUp -= (info.RTT / 1000.0f);

							const auto timeBeforePredictionDisplayed = (bullet.frameToActivateAt - sequenceNumber) * FRAME_DT; 
							const auto bulletCurrentTimeElapsed = bullet.timeElapsed - timeBeforePredictionDisplayed + bullet.timeToCatchUp;
							const auto timeDysnych = spawnPredictedBullet.timeElapsed - bulletCurrentTimeElapsed;
							spawnPredictedBullet.timeToSynchornize = timeDysnych;
							spawnPredictedBullet.tSynchronizaztion = 0.0f;
							
							predictedBullets.pop_back();
						}
					}
					

				}
				bullet.aliveFramesLeft = msgBullet.aliveFramesLeft;
				bullet.ownerPlayerIndex = msgBullet.ownerPlayerIndex;
			}

			break;
		}
 
		case GameMessageType::LEADERBOARD_UPDATE: {
			const auto update = static_cast<LeaderboardUpdateMessage*>(message);
			if (update->GetBlockSize() != update->entryCount * sizeof(LeaderboardUpdateMessage::Entry)) {
				break;
			}
			const auto entries = reinterpret_cast<LeaderboardUpdateMessage::Entry*>(update->GetBlockData());

			std::cout << "{\nleaderboard update\n";
			for (int i = 0; i < update->entryCount; i++) {
				const auto entry = entries[i];
				std::cout << entry.playerIndex << ' ' << entry.kills << ' ' << entry.deaths << '\n';
			}
			std::cout << "}\n";

			for (const auto& [playerIndex, entry] : playerIdToLeaderboardEntry) {
				std::cout << playerIndex << ' ' << entry.kills << ' ' << entry.deaths << '\n';
			}
			for (i32 i = 0; i < update->entryCount; i++) {
				const auto entry = entries[i];

				if (playerIdToLeaderboardEntry[entry.playerIndex].deaths != entry.deaths) {
					std::cout << "died: " << entry.playerIndex << '\n';
					if (entry.playerIndex == clientPlayerIndex) {
						isAlive = false;
						renderer.playDeathAnimation(playerTransform.position);
					} else {
						renderer.playDeathAnimation(playerIndexToTransform[entry.playerIndex].position);
						playerIndexToTransform.erase(entry.playerIndex);
					}
					
				}

				playerIdToLeaderboardEntry[entry.playerIndex] = {
					.kills = entry.kills,
					.deaths = entry.deaths,
				};
			}
			break;
		}

		case GameMessageType::SPAWN_PLAYER: {
			const auto& spawn = *reinterpret_cast<SpawnMessage*>(message);
			std::cout << "spawn player: " << spawn.playerIndex << '\n';
			if (clientPlayerIndex == spawn.playerIndex) {
				isAlive = true;
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
