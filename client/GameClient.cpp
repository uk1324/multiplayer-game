#include <client/GameClient.hpp>
#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <Engine/Engine.hpp>
#include <engine/Math/Utils.hpp>
#include <shared/Networking.hpp>
#include <Types.hpp>
#include <shared/Gameplay.hpp>
#include <engine/Math/Color.hpp>
#include <client/Debug.hpp>
#include <engine/Utils/Put.hpp>
#include <RefOptional.hpp>
#include <engine/Utils/MapOptGet.hpp>

template<typename T>
static float average(const std::vector<T> vs) {
	float s = 0.0f;
	for (const auto& v : vs) {
		s += v;
	}
	return s / vs.size();
}

GameClient::GameClient(yojimbo::Client& client, Renderer& renderer)
	: client(client)
	, renderer(renderer) {

}
 
void gui(const yojimbo::NetworkInfo& info) {
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

	if (!joinedGame()) {
		CHECK_NOT_REACHED();
		return;
	}

	ImGui::TextWrapped("the flickering point is the position of the player from the server update");

	const auto input = [
		cursorPosWorldSpace = Input::cursorPosClipSpace() * renderer.camera.clipSpaceToWorldSpace(),
		clientPlayerPosition = clientPlayer.position
	] {
		const auto cursorRelativeToPlayer = cursorPosWorldSpace - clientPlayerPosition;
		const auto rotation = atan2(cursorRelativeToPlayer.y, cursorRelativeToPlayer.x);
		return ClientInputMessage::Input{
			.up = Input::isKeyHeld(KeyCode::W),
			.down = Input::isKeyHeld(KeyCode::S),
			.left = Input::isKeyHeld(KeyCode::A),
			.right = Input::isKeyHeld(KeyCode::D),
			.shoot = Input::isMouseButtonHeld(MouseButton::LEFT),
			.shift = Input::isKeyHeld(KeyCode::LEFT_SHIFT),
			.rotation = rotation
		};
	}();

	auto processInput = [
		sequenceNumber = sequenceNumber,
		&input,
		newestUpdateLastReceivedClientSequenceNumber = newestUpdateLastReceivedClientSequenceNumber,

		&pastInputs = pastInputs,
		&clientPlayer = clientPlayer
	] {

		pastInputs.push_back({ input, sequenceNumber });
		int toRemoveFromBackCount = 0;
		for (const auto& input : pastInputs) {
			if (input.sequenceNumber <= newestUpdateLastReceivedClientSequenceNumber) {
				toRemoveFromBackCount = 0;
			}
		}
		toRemoveFromBackCount = std::min(toRemoveFromBackCount, ClientInputMessage::INPUTS_COUNT);
		pastInputs.erase(pastInputs.begin(), pastInputs.begin() + toRemoveFromBackCount);

		updateGameplayPlayer(clientPlayer, input, FRAME_DT_SECONDS);
	};
	processInput();

	auto inputMessage = [
		sequenceNumber = sequenceNumber, 
		&pastInputs = std::as_const(pastInputs),

		inputMessage = static_cast<ClientInputMessage*>(client.CreateMessage(GameMessageType::CLIENT_INPUT))
	] {
		inputMessage->clientSequenceNumber = sequenceNumber;
		const auto pastInputsCount = static_cast<int>(pastInputs.size());
		const auto messageOffset = std::max(ClientInputMessage::INPUTS_COUNT - pastInputsCount, 0);
		const auto count = std::min(ClientInputMessage::INPUTS_COUNT, pastInputsCount);
		for (int i = 0; i < count; i++) {
			inputMessage->inputs[messageOffset + i] = pastInputs[pastInputsCount - count + i].input;
		}
		return inputMessage;
	}();
	client.SendMessage(GameChannel::UNRELIABLE, inputMessage);

	auto updatePlayersPositions = [
		&playerIndexToTransform = std::as_const(playerIndexToTransform),
		&clientPlayer = std::as_const(clientPlayer),
		clientPlayerIndex = clientPlayerIndex,

		&players = players
	] {
		players[clientPlayerIndex].position = clientPlayer.position;

		for (auto& [playerIndex, transform] : playerIndexToTransform) {
			/*transform
			players[playerIndex].position = transform.position;*/
		}
	};
	updatePlayersPositions();

	auto addToRender = [
		&players = std::as_const(players),
		clientPlayerIndex = clientPlayerIndex,

		&renderer = renderer
	] {
		auto playerColor = [&](PlayerIndex playerIndex) {
			if (playerIndex == clientPlayerIndex) {
				return Vec3(0.0f, 1.0f, 1.0f);
			} else {
				return Vec3(1.0f, 0.0f, 0.0f);
			}
		};

		/*for (auto& bullet : predictedBullets) {
			renderer.drawSprite(renderer.bulletSprite, bullet.position, BULLET_HITBOX_RADIUS * 2.0f, 0.0f, Vec4(1.0f, 1.0f, 1.0f));
		}*/

		/*for (const auto& animation : renderer.deathAnimations) {
			if (animation.t >= 0.5) {
				players[animation.playerIndex].isRendered = false;
			}
		}*/

		//for (const auto& animation : renderer.spawnAnimations) {
		//	players[animation.playerIndex].isRendered = true;
		//}

		const auto drawPlayer = [&](Vec2 pos, Vec3 color, Vec2 sizeScale) {
			renderer.playerInstances.toDraw.push_back(PlayerInstance{
				.transform = renderer.camera.makeTransform(pos, 0.0f, sizeScale * Vec2(PLAYER_HITBOX_RADIUS / 0.1 /* Read shader */)),
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
					*sneakingElapsed += FRAME_DT_SECONDS;
					sneakingElapsed = std::min(*sneakingElapsed, SNEAKING_ANIMATION_DURATION);
				}
			} else {
				if (sneakingElapsed.has_value()) {
					*sneakingElapsed -= FRAME_DT_SECONDS;
					if (sneakingElapsed <= 0.0f) {
						sneakingElapsed = std::nullopt;
					}
				}
			}
			{
				float t = sneakingElapsed.has_value() ? std::clamp(*sneakingElapsed / SNEAKING_ANIMATION_DURATION, 0.0f, 1.0f) : 0.0f;
				t = smoothstep(t);
				auto color = playerColor(playerIndex);
				color = lerp(color, color / 2.0f, t);
				drawPlayer(player.position, color, sizeScale);
			}
		}

		auto calculateBulletOpacity = [](int aliveFramesLeft) {
			const auto opacityChangeFrames = 60.0f;
			const auto opacity = 1.0f - std::clamp((opacityChangeFrames - aliveFramesLeft) / opacityChangeFrames, 0.0f, 1.0f);
			return opacity;
		};

	};
	renderer.camera.pos = clientPlayer.position;
	Debug::scrollInput(renderer.camera.zoom);
	addToRender();

	sequenceNumber++;
	yojimbo::NetworkInfo info;
	client.GetNetworkInfo(info);
	
	//put("executed: % received: % difference: % RTT: % executed delay: % received delay: %", sequenceNumber + averageExecuteDelay, sequenceNumber + averageReceiveDelay, averageExecuteDelay - averageReceiveDelay, info.RTT, averageExecuteDelay, averageReceiveDelay);
}

void GameClient::processMessage(yojimbo::Message* message) {
	switch (message->GetType()) {
		case GameMessageType::CLIENT_INPUT:
			ASSERT_NOT_REACHED();
			break;

		case GameMessageType::WORLD_UPDATE: { 
			const auto& msg = reinterpret_cast<WorldUpdateMessage&>(*message);

			if (msg.serverSequenceNumber < newestUpdateServerSequenceNumber) {
				put("out of order update message");
				break;
			}
			newestUpdateServerSequenceNumber = msg.serverSequenceNumber;
			newestUpdateLastReceivedClientSequenceNumber = msg.lastExecutedInputClientSequenceNumber;
			
			const auto serverFrame = msg.serverSequenceNumber * SERVER_UPDATE_SEND_RATE_DIVISOR;

			auto updatePastDelaysArrays = [
				serverFrame, 
				sequenceNumber = sequenceNumber,
				&msg = std::as_const(msg),

				&pastReceiveDelays = pastReceiveDelays,
				&pastExecuteDelays = pastExecuteDelays
			] {
				auto addDelay = [](std::vector<FrameTime>& delays, FrameTime newDelay) {
					delays.insert(delays.begin(), newDelay);
					if (delays.size() > 10) {
						delays.pop_back();
					}
				};

				// https://en.wikipedia.org/wiki/Network_Time_Protocol#Clock_synchronization_algorithm
				// Average of the time it takes for the packet to get to the server. The first difference is the time it takes to get to the server the second is how long it takes to get back from the server.
				const auto delayExecute = ((serverFrame - msg.lastExecutedInputClientSequenceNumber) + (serverFrame - sequenceNumber)) / 2.0f; // Should this be using sequence number or something else?
				const auto delayReceive = ((serverFrame - msg.lastReceivedInputClientSequenceNumber) + (serverFrame - sequenceNumber)) / 2.0f;
				addDelay(pastReceiveDelays, delayReceive);
				addDelay(pastExecuteDelays, delayExecute);
				
				// NOTE: The delay between the server and the client seems to grow don't know why.
			};
			updatePastDelaysArrays();
			averageReceiveDelay = average(pastReceiveDelays);
			averageExecuteDelay = average(pastExecuteDelays);
	
			yojimbo::NetworkInfo info;
			client.GetNetworkInfo(info);
			if (info.RTT == 0.0f) {
				// Messages were exchanged between the server and client so the RTT should be calculated.
				CHECK_NOT_REACHED();
				break;
			}

			for (const auto& msgPlayer : msg.players) {
				if (msgPlayer.playerIndex == clientPlayerIndex) {
					clientPlayer.position = msgPlayer.position;
					for (const auto& input : pastInputs) {
						if (input.sequenceNumber > msg.lastExecutedInputClientSequenceNumber) {
							clientPlayer.position = applyMovementInput(clientPlayer.position, input.input, FRAME_DT_SECONDS);
						}
					}
					Debug::drawCircle(msgPlayer.position, PLAYER_HITBOX_RADIUS);
				} else {

				}
			}

			break;
		}
 
		case GameMessageType::LEADERBOARD_UPDATE: {
			break;
		}

		case GameMessageType::SPAWN_PLAYER: {
			
			break;
		}

		case GameMessageType::TEST:
			std::cout << "hit\n";
			break;
	}
}

//double GameClient::time() {
//	using namespace std::chrono;
//	return duration<double>(high_resolution_clock::now().time_since_epoch()).count();
//}

void GameClient::onJoin(const JoinMessage& msg) {
	/*clientPlayerIndex = playerIndex;
	players.insert({ clientPlayerIndex, GameClient::Player{} });
	sequenceNumber = 0;
	joinTime = time();*/
	clientPlayerIndex = msg.clientPlayerIndex;
	//serverTime = serverFrameWithLatency = msg.sentTime;
	put("join clientPlayerIndex = %", clientPlayerIndex);
}

void GameClient::onDisconnected() {
	clientPlayerIndex = -1;
}

bool GameClient::joinedGame() const {
	return clientPlayerIndex != -1;
}