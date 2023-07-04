#include <client/GameClient.hpp>
#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <Engine/Engine.hpp>
#include <engine/Math/Utils.hpp>
#include <shared/Networking.hpp>
#include <Types.hpp>
#include <shared/Gameplay.hpp>
#include <engine/Math/Color.hpp>
#include <RefOptional.hpp>

template<typename Key, typename Value>
std::optional<Value&> map_get(std::unordered_map<Key, Value>& map, const Key& key) {
	const auto it = map.find(key);
	if (it == map.end()) {
		return std::nullopt;
	}
	return it->second;
}

GameClient::GameClient(yojimbo::Client& client, Renderer& renderer)
	: client(client)
	, renderer(renderer) {

}
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

void GameClient::update() {
	if (!client.IsConnected()) {
		CHECK_NOT_REACHED();
		return;
	}
	if (clientPlayerIndex == -1) {
		CHECK_NOT_REACHED();
		return;
	}

	if (Input::isKeyDown(KeyCode::ESCAPE)) {
		disconnect();
		return;
	}

	elapsed += FRAME_DT;

	auto processInput = [this]() {
		const auto cursorPosWorldSpace = Input::cursorPosClipSpace() * renderer.camera.clipSpaceToWorldSpace();
		const auto cursorRelativeToPlayer = cursorPosWorldSpace - playerTransform.position;
		const auto rotation = atan2(cursorRelativeToPlayer.y, cursorRelativeToPlayer.x);
		const auto newInput = ClientInputMessage::Input{
			.up = Input::isKeyHeld(KeyCode::W),
			.down = Input::isKeyHeld(KeyCode::S),
			.left = Input::isKeyHeld(KeyCode::A),
			.right = Input::isKeyHeld(KeyCode::D),
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

		const auto newPos = applyMovementInput(playerTransform.position, newInput, FRAME_DT);
		playerTransform.inputs.push_back(PastInput{
			.input = newInput,
			.sequenceNumber = sequenceNumber
		});
		playerTransform.position = newPos;

		shootCooldown -= FRAME_DT;
		shootCooldown = std::max(0.0f, shootCooldown);

		thisFrameSpawnIndexCounter = 0;
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
	};

	auto updateBullets = [this]() {
		for (auto& bullet : predictedBullets) {
			if (sequenceNumber <= bullet.frameToActivateAt)
				continue;

			auto dt = FRAME_DT;
			if (bullet.timeToSynchornize < 0.0f) {
				bullet.timeToSynchornize = -bullet.timeToSynchornize;
			}

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
				dt -= value * functionStep * bullet.timeToSynchornize;
			}

			updateBullet(bullet.position, bullet.velocity, bullet.timeElapsed, bullet.timeToCatchUp, bullet.aliveFramesLeft, dt);
		}
	};

	auto render = [this]() {
		renderer.camera.pos = playerTransform.position;

		for (auto& bullet : predictedBullets) {
			renderer.drawSprite(renderer.bulletSprite, bullet.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(1.0f, 1.0f, 1.0f));
		}

		for (const auto& animation : renderer.deathAnimations) {
			if (animation.t >= 0.5) {
				players[animation.playerIndex].isRendered = false;
			}
		}

		for (const auto& animation : renderer.spawnAnimations) {
			players[animation.playerIndex].isRendered = true;
		}

		const auto drawPlayer = [this](Vec2 pos, Vec3 color, Vec2 sizeScale) {
			renderer.playerInstances.toDraw.push_back(PlayerInstance{
				.transform = renderer.camera.makeTransform(pos, 0.0f, sizeScale * Vec2(PLAYER_HITBOX_RADIUS / 0.1 /* Read shader */)),
				.time = elapsed,
				.color = color,
			});
		};

		for (const auto& [playerIndex, player] : players) {
			if (!player.isRendered)
				continue;

			const auto& sprite = renderer.bulletSprite;
			Vec2 sizeScale(1.0f);
			float opacity = 1.0f;
			//Vec4 color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
			for (const auto& animation : renderer.spawnAnimations) {
				if (playerIndex == animation.playerIndex) {
					auto t = animation.t;
					sizeScale.y *= lerp(2.0f, 1.0f, t);
					sizeScale.x *= lerp(0.0f, 1.0f, t); // Identity function lol
					opacity = t;
					break;
				}
			}	
			//renderer.drawSprite(sprite, player.position, size, 0.0f, color);
			const auto SNEAKING_ANIMATION_DURATION = 0.3f;
			static std::optional<float> sneakingElapsed;
			if (Input::isKeyHeld(KeyCode::LEFT_SHIFT)) {
				if (!sneakingElapsed.has_value()) {
					sneakingElapsed = 0.0f;
				} else {
					*sneakingElapsed += FRAME_DT;
					sneakingElapsed = std::min(*sneakingElapsed, SNEAKING_ANIMATION_DURATION);
				}
			} else {
				if (sneakingElapsed.has_value()) {
					*sneakingElapsed -= FRAME_DT;
					if (sneakingElapsed <= 0.0f) {
						sneakingElapsed = std::nullopt;
					}
				}
			}
			{
				float t = sneakingElapsed.has_value() ? std::clamp(*sneakingElapsed / SNEAKING_ANIMATION_DURATION, 0.0f, 1.0f) : 0.0f;
				t = smoothstep(t);
				auto color = getPlayerColor(playerIndex);
				color = lerp(color, color / 2.0f, t);
				drawPlayer(player.position, color, sizeScale);
			}
		}

		auto calculateBulletOpacity = [](int aliveFramesLeft) {
			const auto opacityChangeFrames = 60.0f;
			const auto opacity = 1.0f - std::clamp((opacityChangeFrames - aliveFramesLeft) / opacityChangeFrames, 0.0f, 1.0f);
			return opacity;
		};

		//for (auto& [_, bullet] : interpolatedBullets) {
		//	const auto opacity = calculateBulletOpacity(bullet.aliveFramesLeft);
		//	renderer.drawSprite(renderer.bulletSprite, bullet.transform.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(1.0f, 1.0f, 1.0f, opacity));
		//}
	};

	if (isAlive) {
		processInput();
	} else {
		if (Input::isKeyDown(KeyCode::SPACE)) {
			const auto message = client.CreateMessage(GameMessageType::SPAWN_REQUEST);
			client.SendMessage(GameChannel::RELIABLE, message);
		}
	}

	// Debug
	{
		for (auto& [_, bullet] : interpolatedBullets) {
			bullet.transform.interpolatePosition(sequenceNumber);
			bullet.aliveFramesLeft--;
		}
		std::erase_if(interpolatedBullets, [](const auto& item) { return item.second.aliveFramesLeft <= 0; });
	}

	for (auto& [index, transform] : playerIndexToTransform) {
		transform.interpolatePosition(sequenceNumber);
		players[index].position = transform.position;
	}
	players[clientPlayerIndex].position = playerTransform.position;

	updateBullets();

	render();
	
	sequenceNumber++;
}

void GameClient::processMessage(yojimbo::Message* message) {
	switch (message->GetType()) {
		

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

			/*const auto playersSize = msg->playersCount * sizeof(WorldUpdateMessage::Player);
			const auto bulletsSize = msg->bulletsCount * sizeof(WorldUpdateMessage::Bullet);
			const auto dataSize = playersSize + bulletsSize;

			if (msg->GetBlockSize() != dataSize) {
				ASSERT_NOT_REACHED();
				break;
			}*/
			newestUpdateSequenceNumber = msg->sequenceNumber;
			newestUpdateLastReceivedClientSequenceNumber = msg->lastReceivedClientSequenceNumber;

			/*const auto msgPlayers = reinterpret_cast<WorldUpdateMessage::Player*>(msg->GetBlockData());
			const auto msgBullets = reinterpret_cast<WorldUpdateMessage::Bullet*>(msg->GetBlockData() + playersSize);*/

			for (const auto& msgPlayer : msg->players) {
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
						playerTransform.position = applyMovementInput(playerTransform.position, prediction.input, FRAME_DT);
					}
				} else {
					auto& transform = playerIndexToTransform[msgPlayer.index];
					transform.updatePositions(firstUpdate, msgPlayer.position, sequenceNumber, msg->sequenceNumber);
				}
			}

			for (const auto& msgBullet : msg->bullets) {
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

			//std::cout << "{\nleaderboard update\n";
			//for (int i = 0; i < update->entryCount; i++) {
			//	const auto entry = entries[i];
			//	std::cout << entry.playerIndex << ' ' << entry.kills << ' ' << entry.deaths << '\n';
			//}
			//std::cout << "}\n";

			//for (const auto& [playerIndex, entry] : playerIdToLeaderboardEntry) {
			//	std::cout << playerIndex << ' ' << entry.kills << ' ' << entry.deaths << '\n';
			//}
			for (i32 i = 0; i < update->entryCount; i++) {
				const auto entry = entries[i];

				auto player = map_get(players, entry.playerIndex);
				if (!player.has_value()) {
					CHECK_NOT_REACHED();
					continue;
				}
				
				if (const auto died = player->leaderboard.deaths != entry.deaths) {
					//std::cout << "died: " << entry.playerIndex << '\n';
					renderer.deathAnimations.push_back(Renderer::DeathAnimation{
						.position = player->position,
						.color = getPlayerColor(entry.playerIndex),
						.playerIndex = entry.playerIndex,
					});
					if (entry.playerIndex == clientPlayerIndex) {
						isAlive = false;
					}
				}

				player->leaderboard = {
					.kills = entry.kills,
					.deaths = entry.deaths,
				};
			}
			break;
		}

		case GameMessageType::SPAWN_PLAYER: {
			const auto& spawn = *reinterpret_cast<SpawnMessage*>(message);
			std::cout << "spawn player: " << spawn.playerIndex << '\n';
			renderer.spawnAnimations.push_back(Renderer::SpawnAnimation{ .playerIndex = spawn.playerIndex });
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

void GameClient::onJoin(int playerIndex) {
	clientPlayerIndex = playerIndex;
	players.insert({ clientPlayerIndex, GameClient::Player{} });
	sequenceNumber = 0;
}

bool GameClient::joinedGame() {
	return clientPlayerIndex != -1;
}

void GameClient::disconnect() {
	client.Disconnect();
	clientPlayerIndex = -1;
}

Vec3 GameClient::getPlayerColor(int id) {
	if (id == clientPlayerIndex) {
		return Vec3(0.0f, 1.0f, 1.0f);
	} else {
		return Vec3(1.0f, 0.0f, 0.0f);
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