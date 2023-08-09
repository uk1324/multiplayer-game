#include <client/GameClient.hpp>
#include <engine/Window.hpp>
#include <engine/Input/Input.hpp>
#include <engine/Engine.hpp>
#include <engine/Math/Utils.hpp>
#include <shared/Networking.hpp>
#include <Types.hpp>
#include <shared/Gameplay.hpp>
#include <engine/Math/Color.hpp>
#include <client/Debug.hpp>
#include <client/ClientGameplayContext.hpp>
#include <engine/Utils/Put.hpp>
#include <RefOptional.hpp>
#include <engine/Utils/MapOptGet.hpp>
#include <engine/Json/JsonPrinter.hpp>
#include <utility>

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
	, renderer(renderer)
	, clientPlayerIndex(-1) {}

GameClient::GameClient(const JoinMessage& join, GameClient&& old)
	: clientPlayerIndex(join.clientPlayerIndex)
	, client(old.client)
	, renderer(old.renderer)
	, players(std::move(old.players)) {
	players.clear();

	clientPlayer.position = Vec2(0.0f);
	for (const auto& player : join.players) {
		addJoinMessagePlayer(player);
		//playerIndexToTransform[player.playerIndex].position = player
	}

	#ifdef DEBUG_REPLAY_RECORDER
	replayRecorder.outputFile += std::to_string(clientPlayerIndex);
	#endif

	put("join clientPlayerIndex = %", clientPlayerIndex);
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

	if (Input::isKeyHeld(KeyCode::TAB)) {
		using namespace ImGui;

		auto& style = GetStyle();
		auto& io = GetIO();
		SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		SetNextWindowSize(ImVec2(0.0f, 0.0f));

		PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		auto background = GetStyleColorVec4(ImGuiCol_WindowBg);
		background.w = 0.5f;
		PushStyleColor(ImGuiCol_WindowBg, background);

		Begin("leaderboard", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
		PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(42.0f, 12.0f));

		if (BeginTable("table1", 3, ImGuiTableFlags_BordersH)) {

			TableNextRow();

			TableNextColumn();
			Text("player index");
			TableNextColumn();
			Text("kills");
			TableNextColumn();
			Text("deaths");

			for (const auto& [playerIndex, player] : players) {
				TableNextRow();

				TableNextColumn();
				Text("%d", playerIndex);
				TableNextColumn();
				Text("%d", player.leaderboard.kills);
				TableNextColumn();
				Text("%d", player.leaderboard.deaths);
			}

			EndTable();
		}
		PopStyleVar();

		End();
		PopStyleVar(2);
		PopStyleColor();
	}
	

	#ifdef DEBUG_REPLAY_RECORDER
	if (replayRecorder.isRecording) {
		replayRecorder.addFrame({ clientPlayer }, gameplayState);
	}
	#endif

	const auto input = [
		cursorPosWorldSpace = Input::cursorPosClipSpace() * renderer.camera.clipSpaceToWorldSpace(),
		clientPlayerPosition = clientPlayer.position,

		&selectedPattern = selectedPattern
	] {
		auto newPattern = selectedPattern;
		if (Input::isKeyDown(KeyCode::ALPHA_1)) newPattern = 0;
		else if (Input::isKeyDown(KeyCode::ALPHA_2)) newPattern = 1;
		else if (Input::isKeyDown(KeyCode::ALPHA_3)) newPattern = 2;
		else if (Input::isKeyDown(KeyCode::ALPHA_4)) newPattern = 3;
		else if (Input::isKeyDown(KeyCode::ALPHA_5)) newPattern = 4;
		else if (Input::isKeyDown(KeyCode::ALPHA_6)) newPattern = 5;
		else if (Input::isKeyDown(KeyCode::ALPHA_7)) newPattern = 6;
		else if (Input::isKeyDown(KeyCode::ALPHA_8)) newPattern = 7;
		else if (Input::isKeyDown(KeyCode::ALPHA_9)) newPattern = 8;
		if (newPattern < std::size(patternInfos)) {
			selectedPattern = newPattern;
		}

		const auto cursorRelativeToPlayer = cursorPosWorldSpace - clientPlayerPosition;
		const auto rotation = atan2(cursorRelativeToPlayer.y, cursorRelativeToPlayer.x);
		return ClientInputMessage::Input{
			.up = Input::isKeyHeld(KeyCode::W),
			.down = Input::isKeyHeld(KeyCode::S),
			.left = Input::isKeyHeld(KeyCode::A),
			.right = Input::isKeyHeld(KeyCode::D),
			.shoot = Input::isMouseButtonHeld(MouseButton::LEFT),
			.shift = Input::isKeyHeld(KeyCode::LEFT_SHIFT),
			.rotation = rotation,
			.selectedPattern = selectedPattern
		};
	}();

	{
		auto player = get(players, clientPlayerIndex);
		if (player.has_value()) {
			if (!player->isAlive && Input::isKeyDown(KeyCode::SPACE)) {
				sendSpawnRequest();
			}
		} else {
			CHECK_NOT_REACHED();
		}
	}

	auto activateInactiveBullets = [
		sequenceNumber = sequenceNumber,

		&gameplayState = gameplayState,
		&inactiveGameplayState = inactiveGameplayState
	] {
		std::erase_if(inactiveGameplayState.moveForwardBullets, [&](const auto& it) {
			if (sequenceNumber >= it.second.synchronization.frameToActivateAt) {
				//put("spawned current frame: % should be spawned at: %", sequenceNumber, it.second.synchronization.frameToActivateAt);
				gameplayState.moveForwardBullets.insert(it);
				return true;
			}
			return false;
		});
		//dbg(inactiveGameplayState.moveForwardBullets.size());
	};
	activateInactiveBullets();

	auto processInput = [
		clientPlayerIndex = clientPlayerIndex,
		sequenceNumber = sequenceNumber,
		&input,
		newestUpdateLastReceivedClientSequenceNumber = newestUpdateLastReceivedClientSequenceNumber,
		players = std::as_const(players),

		&pastInputs = pastInputs,
		&clientPlayer = clientPlayer,
		&gameplayState = gameplayState
	] {

		const auto player = get(players, clientPlayerIndex);
		if (player.has_value() && !player->isAlive) {
			return;
		}

		pastInputs.push_back({ input, sequenceNumber });
		int toRemoveFromBackCount = 0;
		for (const auto& input : pastInputs) {
			if (input.sequenceNumber <= newestUpdateLastReceivedClientSequenceNumber) {
				toRemoveFromBackCount = 0;
			}
		}
		toRemoveFromBackCount = std::min(toRemoveFromBackCount, ClientInputMessage::INPUTS_COUNT);
		pastInputs.erase(pastInputs.begin(), pastInputs.begin() + toRemoveFromBackCount);

		updateGameplayPlayer(clientPlayerIndex, clientPlayer, gameplayState, input, sequenceNumber, FRAME_DT_SECONDS);
	};

	updateGemeplayStateBeforeProcessingInput(gameplayState);
	processInput();
	ClientGameplayContext context(*this);
	updateGameplayStateAfterProcessingInput(gameplayState, context, FRAME_DT_SECONDS);

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
		&clientPlayer = std::as_const(clientPlayer),
		clientPlayerIndex = clientPlayerIndex,
		sequenceNumber = sequenceNumber,
		&delays = std::as_const(delays),

		&playerIndexToTransform = playerIndexToTransform,
		&players = players
	] {
		players[clientPlayerIndex].position = clientPlayer.position;
		
		if (delays.has_value()) {
			for (auto& [playerIndex, transform] : playerIndexToTransform) {
				transform.interpolatePosition(sequenceNumber, *delays);
				players[playerIndex].position = transform.position;
			}
		}
	};
	updatePlayersPositions();

	auto addToRender = [
		&players = std::as_const(players),
		clientPlayerIndex = clientPlayerIndex,
		&gameplayState = std::as_const(gameplayState),
		
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

		for (const auto& [index, bullet] : gameplayState.moveForwardBullets) {
			const auto opacityChangeSeconds = 1.0f;
			const auto opacity = 1.0f - std::clamp((bullet.elapsed - (BULLET_ALIVE_SECONDS - opacityChangeSeconds)) / opacityChangeSeconds, 0.0f, 1.0f);

			renderer.bullet.instances.toDraw.push_back(BulletInstance{
				// * 1.3f = Actual bullet size 
				/*.transform = renderer.camera.makeTransform(bullet.position, 0.0f, Vec2(BULLET_HITBOX_RADIUS) * 1.3f),*/
				.transform = renderer.camera.makeTransform(bullet.position, 0.0f, Vec2(BULLET_HITBOX_RADIUS) * 1.7f),
				.color = Vec4(playerColor(index.ownerPlayerIndex), opacity),
			});
		}

		/*const auto drawPlayer = [&](Vec2 pos, Vec3 color, Vec2 sizeScale) {
			
		};*/

		auto drawPlayer = [&](PlayerIndex playerIndex, Vec2 pos, Vec2 scale) {
			renderer.player.instances.toDraw.push_back(PlayerInstance{
				.transform = renderer.camera.makeTransform(pos, 0.0f, scale * Vec2(PLAYER_HITBOX_RADIUS / 0.1f /* Read shader */)),
				.color = playerColor(playerIndex),
			});
		};

		for (const auto& [playerIndex, player] : players) {
			if (!player.isAlive)
				continue;

			Vec2 sizeScale(1.0f);
			for (const auto& animation : renderer.spawnAnimations) {
				if (playerIndex == animation.playerIndex) {
					auto t = animation.t;
					sizeScale.y *= lerp(2.0f, 1.0f, t);
					sizeScale.x *= lerp(0.0f, 1.0f, t); // Identity function lol
					break;
				}
			}

			drawPlayer(playerIndex, player.position, sizeScale);
			////renderer.drawSprite(sprite, player.position, size, 0.0f, color);
			//const auto SNEAKING_ANIMATION_DURATION = 0.3f;
			//static std::optional<float> sneakingElapsed;
			//if (Input::isKeyHeld(KeyCode::LEFT_SHIFT)) {
			//	if (!sneakingElapsed.has_value()) {
			//		sneakingElapsed = 0.0f;
			//	} else {
			//		*sneakingElapsed += FRAME_DT_SECONDS;
			//		sneakingElapsed = std::min(*sneakingElapsed, SNEAKING_ANIMATION_DURATION);
			//	}
			//} else {
			//	if (sneakingElapsed.has_value()) {
			//		*sneakingElapsed -= FRAME_DT_SECONDS;
			//		if (sneakingElapsed <= 0.0f) {
			//			sneakingElapsed = std::nullopt;
			//		}
			//	}
			//}
			//{
			//	float t = sneakingElapsed.has_value() ? std::clamp(*sneakingElapsed / SNEAKING_ANIMATION_DURATION, 0.0f, 1.0f) : 0.0f;
			//	t = smoothstep(t);
			//	auto color = playerColor(playerIndex);
			//	if (playerIndex == clientPlayerIndex) {
			//		color = lerp(color, color / 2.0f, t);
			//	}
			//	drawPlayer(player.position, color, sizeScale);
			//}
		}

		for (const auto& animation : renderer.deathAnimations) {
			const auto l0 = 0.5f;
			if (animation.t <= l0) {
				const auto t = animation.t / 0.5f;
				drawPlayer(animation.playerIndex, animation.position, Vec2(1.0f - t));
			}
			
			renderer.deathAnimation.instances.toDraw.push_back(DeathAnimationInstance{
				.transform = renderer.camera.makeTransform(animation.position, 0.0f, Vec2{ 7.0f * PLAYER_HITBOX_RADIUS }),
				.color = playerColor(animation.playerIndex),
				.time = animation.t,
			});
		}
	};

	cameraFollower.update(renderer.camera, clientPlayer.position);
	//Debug::scrollInput(renderer.camera.zoom);
	/*chk(debugDraw) {
		Vec3 color = Color3::RED;

		for (const auto& [playerIndex, player] : players) {
			Debug::drawCircle(player.position, PLAYER_HITBOX_RADIUS, color);
		}

		for (const auto& [_, bullet] : gameplayState.moveForwardBullets) {
			Debug::drawCircle(bullet.position, BULLET_HITBOX_RADIUS, color);
		}
	} else {
		addToRender();
	}*/
	addToRender();

	cooldownTimer.updateAndRender(renderer, clientPlayer.cooldown, selectedPattern);
	{
		const auto player = get(players, clientPlayerIndex);
		if (player.has_value()) {
			respawnText.updateAndRender(renderer, !player->isAlive);
		} else {
			CHECK_NOT_REACHED();
		}
	}
	drawMap(renderer, players, clientPlayerIndex);

	#ifdef DEBUG_INTERPOLATE_BULLETS
	if (delays.has_value()) {
		for (auto& [_, bullet] : debugInterpolatedBullets) {
			bullet.interpolatePosition(sequenceNumber, *delays);
			Debug::drawCircle(bullet.position, BULLET_HITBOX_RADIUS);
		}
	}
	#endif 

	if (delays.has_value()) {
		//ImGui::TextWrapped("difference between actual and used clock time %d", delays->receiveDelay - averageReceiveDelay);
	}

	yojimbo::NetworkInfo info;
	client.GetNetworkInfo(info);
	gui(info);

	sequenceNumber++;
	/*yojimbo::NetworkInfo info;
	client.GetNetworkInfo(info);
	put("executed: % received: % difference: % RTT: % executed delay: % received delay: %", sequenceNumber + averageExecuteDelay, sequenceNumber + averageReceiveDelay, averageExecuteDelay - averageReceiveDelay, info.RTT, averageExecuteDelay, averageReceiveDelay);*/
}

void GameClient::sendSpawnRequest() {
	auto msg = reinterpret_cast<SpawnRequestMessage*>(client.CreateMessage(GameMessageType::SPAWN_REQUEST));
	client.SendMessage(GameChannel::RELIABLE, msg);
}

void GameClient::addJoinMessagePlayer(const JoinMessagePlayer& player){
	players.insert({ player.playerIndex, Player{ .leaderboard = player.leaderboard, .isAlive = player.isAlive } });
}

void GameClient::processMessage(yojimbo::Message* message) {
	switch (message->GetType()) {
	case GameMessageType::CLIENT_INPUT:
		ASSERT_NOT_REACHED();
		break;

	case GameMessageType::WORLD_UPDATE: { 
		const auto& msg = reinterpret_cast<WorldUpdateMessage&>(*message);

		// TODO: For interpolated positions could update it with older updates, because they might not have been displayed yet.
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
			//put("new delay %", delayReceive);
				
			// NOTE: The delay between the server and the client seems to grow don't know why.
		};
		updatePastDelaysArrays();
		averageReceiveDelay = average(pastReceiveDelays);
		averageExecuteDelay = average(pastExecuteDelays);
	
		yojimbo::NetworkInfo networkInfo;
		client.GetNetworkInfo(networkInfo);
		// RTT is required to calculate the display delay.
		if (networkInfo.RTT == 0.0f) {
			// Messages were exchanged between the server and client so the RTT should be calculated.
			CHECK_NOT_REACHED();
			break;
		}

		auto secondsToFrames = [](float seconds) -> FrameTime {
			return static_cast<FrameTime>(ceil(seconds / FRAME_DT_SECONDS));
		};

		const auto rttSeconds = networkInfo.RTT / 1000.0f;
		//if (!delays.has_value()) {
		//	/*
		//	server frame = client frame + receive delay
		//	client frame = server frame - receive delay

		//	client time when message received = message server frame - receive delay + RTT/2

		//	client time when next message received = message server frame - receive delay + RTT/2 + delay between server updates
		//	delay = -receive delay + RTT/2 + delay between server updates
		//	*/
		//	const auto additionalDelayToHandleJitter = 6;
		//	delays = Delays{
		//		// TODO: Maybe later calculate the delay based on if there are enought positions to interpolate between. If the queues are starving the increase the delay. If the jitter is zero you wouldn't need to do that.
		//		.interpolatedEntitesDisplayDelay = -averageReceiveDelay + secondsToFrames(rttSeconds / 2.0f) + SERVER_UPDATE_SEND_RATE_DIVISOR + additionalDelayToHandleJitter,
		//		.receiveDelay = averageReceiveDelay,
		//		.executeDelay = averageExecuteDelay
		//	};
		//} else {
		//	// TODO: Check if synchronized.
		//}
		{
			const auto additionalDelayToHandleJitter = 6;
			delays = Delays{
				// TODO: Maybe later calculate the delay based on if there are enought positions to interpolate between. If the queues are starving the increase the delay. If the jitter is zero you wouldn't need to do that.
				.interpolatedEntitesDisplayDelay = -averageReceiveDelay + secondsToFrames(rttSeconds / 2.0f) + SERVER_UPDATE_SEND_RATE_DIVISOR + additionalDelayToHandleJitter,
				.receiveDelay = averageReceiveDelay,
				.executeDelay = averageExecuteDelay
			};
		}

		#ifdef DEBUG_INTERPOLATION
			put("time before frame is displayed: %", (serverFrame + delays->interpolatedEntitesDisplayDelay) - sequenceNumber);
		#endif

		for (const auto& msgPlayer : msg.players) {
			if (msgPlayer.playerIndex == clientPlayerIndex) {
				clientPlayer.position = msgPlayer.position;
				for (const auto& input : pastInputs) {
					if (input.sequenceNumber > msg.lastExecutedInputClientSequenceNumber) {
						clientPlayer.position = applyMovementInput(clientPlayer.position, input.input, FRAME_DT_SECONDS);
					}
				}
			} else {
				playerIndexToTransform[msgPlayer.playerIndex].updatePositions(msgPlayer.position, serverFrame);
			}
		}

		#ifdef DEBUG_INTERPOLATE_BULLETS
			#define DEBUG_ADD_INTERPOLATED_BULLET(position) \
				debugInterpolatedBullets[bulletIndex.untypedIndex()].updatePositions(position, serverFrame)
		#else
			#define DEBUG_ADD_INTERPOLATED_BULLET(position)
		#endif

		//put("size %", msg.gemeplayState.moveForwardBullets.size());
		ASSERT(delays.has_value());
		for (const auto& [bulletIndex, msgBullet] : msg.gemeplayState.moveForwardBullets) {
			const auto frameWhenTheBulletInterpolatedBulletWouldBeDisplayed = serverFrame + delays->interpolatedEntitesDisplayDelay;
			// TODO: Doesn't handle negative values.
			const auto timeBeforePredictionDisplayed = (frameWhenTheBulletInterpolatedBulletWouldBeDisplayed - sequenceNumber) * FRAME_DT_SECONDS;

			if (bulletIndex.ownerPlayerIndex == clientPlayerIndex) {
				DEBUG_ADD_INTERPOLATED_BULLET(msgBullet.position);
				const auto i = bulletIndex.untypedIndex();
				auto spawnPredictedBullet = get(gameplayState.moveForwardBullets, bulletIndex);
				if (!spawnPredictedBullet.has_value()) {
					// If the client spawned the bullet then they should have it.
					// CHECK_NOT_REACHED();
					// Except if it got destroyed already.
					continue;
				}
				// @Hack: checking for zero
				if (spawnPredictedBullet->synchronization.timeToSynchronize != 0.0f) {
					continue;
				}
				const auto bulletCurrentTimeElapsed = msgBullet.elapsed - timeBeforePredictionDisplayed /* + msgBullet.timeToCatchUp*/;
				auto timeDesynch = spawnPredictedBullet->elapsed - bulletCurrentTimeElapsed;
				timeDesynch += FRAME_DT_SECONDS; // I think this might need to be added because the bullet will get updated this frame and interpolated versions wont.
				//timeDesynch = timeDesynch;

				//spawnPredictedBullet->position -= spawnPredictedBullet->velocity * timeDysnych;
				spawnPredictedBullet->synchronization.timeToSynchronize = timeDesynch;
				spawnPredictedBullet->synchronization.synchronizationProgressT = 0.0f;
			} else {
				DEBUG_ADD_INTERPOLATED_BULLET(msgBullet.position);
				if (gameplayState.moveForwardBullets.contains(bulletIndex)) {
					continue;
				}
				// The bullet should be synchronized with the server bullet time + half RTT. So the player when a player does an action the the see the same state the server is going to see on the frame the input gets executed on the server.
				/*const auto whenOnClientReceivedServerTime = serverFrame * FRAME_DT_SECONDS + rttSeconds / 2.0f;
				const auto whenDisplayed*/

				const auto timeDesynch = timeBeforePredictionDisplayed + rttSeconds + std::max(0, averageExecuteDelay - averageReceiveDelay) * FRAME_DT_SECONDS;
				auto bullet = msgBullet;

				const auto maxFrameSpeedup = FRAME_DT_SECONDS * 2.0f;
				// maxFrameSpeedup = catchUpPercentPerFrame * timeToCatchUp
				// catchUpPercentPerFrame = maxFrameSpeedup / timeToCatchUp

				// TODO: Could use a std::max for maxFrameSpeedup so it doesn't always accelerate that quickly on the first frame. Or maybe it is more balanced this way.

				// The bullets should as close to being shot out of the interpolated entity. Technically the server update most of the time won't be sent on the frame the bullet was spawned.
				// TODO: This doesn't deal with the case when the frameToActivateAt already passed.
				bullet.synchronization.frameToActivateAt = serverFrame + delays->interpolatedEntitesDisplayDelay;
				bullet.synchronization.timeToCatchUp = timeDesynch;
				bullet.synchronization.catchUpPercentPerFrame = maxFrameSpeedup / bullet.synchronization.timeToCatchUp;
				inactiveGameplayState.moveForwardBullets.insert({ bulletIndex, bullet });
			}
		}

		break;
	}
 
	case GameMessageType::LEADERBOARD_UPDATE: {
		const auto& msg = reinterpret_cast<LeaderboardUpdateMessage*>(message);
		for (const auto& [playerIndex, entry] : msg->entries) {
			auto player = get(players, playerIndex);
			if (!player.has_value()) {
				CHECK_NOT_REACHED();
				continue;
			}
			if (const auto died = player->leaderboard.deaths != entry.deaths) {
				if (!player->isAlive) {
					CHECK_NOT_REACHED();
				}
				player->isAlive = false;
				renderer.deathAnimations.push_back(Renderer::DeathAnimation{
					.position = player->position,
					.playerIndex = playerIndex,
				});
			}
			player->leaderboard = entry;

		}
		break;
	}

	case GameMessageType::SPAWN_PLAYER: {
		const auto msg = reinterpret_cast<SpawnPlayerMessage*>(message);
		put("spawning %", msg->playerIndex);
		auto player = get(players, msg->playerIndex);
		if (!player.has_value()) {
			CHECK_NOT_REACHED();
			return;
		}
		player->isAlive = true;
		renderer.spawnAnimations.push_back(Renderer::SpawnAnimation{
			.playerIndex = msg->playerIndex,
		});
		if (msg->playerIndex == clientPlayerIndex) {
			clientPlayer.position = msg->position;
			cameraFollower.state = CameraFollower::State::PAN_TO_PLAYER;
		} else {
			// @Hack
			auto& transform = playerIndexToTransform[msg->playerIndex];
			transform.positions.clear();
			transform.position = msg->position;
		}
		break;
	}

	case GameMessageType::PLAYER_JOINED: {
		const auto msg = reinterpret_cast<PlayerJoinedMessage*>(message);
		addJoinMessagePlayer(msg->player);
		break;
	}

	case GameMessageType::PLAYER_DISCONNECTED: {
		// Maybe play death animation.
		const auto msg = reinterpret_cast<PlayerDisconnectedMessage*>(message);
		players.erase(msg->playerIndex);
		playerIndexToTransform.erase(msg->playerIndex);
		break;
	}

	case GameMessageType::TEST:
		break;
	}
}

void GameClient::InterpolatedTransform::updatePositions(Vec2 newPosition, FrameTime serverFrame) {
	positions.push_back(InterpolatedTransform::Position{
		.position = newPosition,
		.serverFrame = serverFrame
	});
}

void GameClient::InterpolatedTransform::interpolatePosition(FrameTime sequenceNumber, const Delays& delays) {
	#ifdef DEBUG_INTERPOLATION
		ImGui::Text("current time");
		Debug::text(sequenceNumber);
		ImGui::Text("display at times");
		for (const auto& position : positions) {
			Debug::text(position.serverFrame + delays.interpolatedEntitesDisplayDelay);
		}
	#endif

	if (positions.size() == 1) {
		position = positions[0].position;
	} else if (positions.size() > 1) {
		int i = 0;
		for (i = 0; i < positions.size() - 1; i++) {
			auto frameToDisplayAt = [&](const InterpolatedTransform::Position& position) -> FrameTime {
				return position.serverFrame + delays.interpolatedEntitesDisplayDelay;
			};
			const auto iFrameTodisplayAt = frameToDisplayAt(positions[i]);
			const auto iPlusOneFrameTodisplayAt = frameToDisplayAt(positions[i + 1]);
			if (iFrameTodisplayAt <= sequenceNumber && iPlusOneFrameTodisplayAt > sequenceNumber) {
				auto t =
					static_cast<float>(sequenceNumber - iFrameTodisplayAt) /
					static_cast<float>(iPlusOneFrameTodisplayAt - iFrameTodisplayAt);
				t = std::clamp(t, 0.0f, 1.0f);

				const auto start = positions[i].position;
				const auto end = positions[i + 1].position;
				position = lerp(start, end, t);
				// Can't use hermite interpolation because it overshoots, which makes it look like it's rubber banding.
				// TODO: The overhsooting might not happen if I store more frames, but this would also add more latency. But I don't think that would actually fix that.
				// https://gdcvault.com/play/1024597/Replicating-Chaos-Vehicle-Replication-in

				if (positions.size() > 2) {
					positions.erase(positions.begin(), positions.begin() + i);
				}

				break;
			}
		}
	}
}

void GameClient::CameraFollower::update(Camera& camera, Vec2 playerPosition) {
	using enum GameClient::CameraFollower::State;
	switch (state)
	{
	case FOLLOW_PLAYER:
		camera.pos = playerPosition;
		break;
	case PAN_TO_PLAYER:
		const auto toPlayerPos = playerPosition - camera.pos;
		const auto distance = toPlayerPos.length();
		auto move = 0.07f * distance;
		move = std::max(PLAYER_SPEED * FRAME_DT_SECONDS + 0.01f, move);
		move = std::min(distance, move);
		camera.pos += move * (toPlayerPos / distance);
		if (camera.pos.distanceTo(playerPosition) < 0.01f) {
			state = FOLLOW_PLAYER;
		}
		break;
	}
}


// Height from -0.5 to 0.5
// Width is scaled by aspect ratio
static Mat3x2 transformRelativeToAnchor(const Camera& camera, Vec2 screenAnchor) {
	// To go from [-0.5, 0.5] to [-1, 1].
	const auto scale = 2.0f;
	const auto toScreenAnchor = Mat3x2::translate(screenAnchor * scale);
	const auto screenScale = Mat3x2::scale(Vec2(1.0f / camera.aspectRatio, 1.0f)) * Mat3x2::scale(Vec2(scale));
	return screenScale * toScreenAnchor;
}

float percentToWidth(const Camera& camera, float percent) {
	return percent * camera.aspectRatio;
}

void GameClient::CooldownTimer::updateAndRender(Renderer& renderer, const PlayerPatternCooldowns& cooldown, i32 currentSelectedPattern) {
	auto drawCooldownTimer = [
		&renderer,
		&displayedCooldownTs = std::as_const(displayedCooldownTs)
	](i32 patternType, float movingDownProgress) {

		const Vec2 size(percentToWidth(renderer.camera, 0.35f), 0.065f);
		const auto objectAnchor = -(size / 2.0f);
		Vec2 pos(0.02f);

		const auto text = patternInfos[patternType].name;
		const auto textHeight = 0.025f;
		const auto textInfo = renderer.getTextInfo(renderer.font, textHeight, text);
		const auto textPosOffset = Vec2(0.03f, -textInfo.bottomY + size.y + 0.01f);

		const auto height = pos.y + textPosOffset.y + textInfo.size.y;

		pos.y -= height * movingDownProgress;

		Vec2 textPos = pos + textPosOffset;

		const auto bottomLeftCorner = Vec2(-0.5f, -0.5f);
		const auto anchorTransform = transformRelativeToAnchor(renderer.camera, bottomLeftCorner);
		const auto objectTransform = (Mat3x2::scale(size / 2.0f) * Mat3x2::translate(-objectAnchor + pos)) * anchorTransform;

		float t = 1.0f - displayedCooldownTs[patternType];
		renderer.cooldownTimer.instances.toDraw.push_back(CooldownTimerInstance{
			.transform = objectTransform,
			.t = t,
			.size = size
		});
		renderer.addTextToDraw(renderer.text.instances, renderer.font, textPos, anchorTransform, textHeight, text);
	};
	const auto betweenTransitions = 0.25;
	const auto hidingTime = 0.5f - betweenTransitions;
	const auto hidingTimeLength = hidingTime;
	const auto showingTime = 0.5 + betweenTransitions;
	const auto showingTimeLength = hidingTimeLength;
	if (previousFrameSelectedPattern != currentSelectedPattern) {
		if (selectedPatternTransition.has_value()) {
			const auto t = selectedPatternTransition->t;
			if (const auto newNotDisplayedYet = t <= showingTime) {
				selectedPatternTransition->newPattern = currentSelectedPattern;
			} else {
				selectedPatternTransition = SelectedPatternTransition{
					.t = 1.0f - selectedPatternTransition->t,
					.currentPattern = selectedPatternTransition->newPattern,
					.newPattern = currentSelectedPattern,
				};
			}
		} else {
			selectedPatternTransition = SelectedPatternTransition{
				.t = 0.0f,
				.currentPattern = previousFrameSelectedPattern,
				.newPattern = currentSelectedPattern
			};
		}
	}

	for (i32 patternType = 0; patternType < PatternType::PatternType::COUNT; patternType++) {
		const auto actualCooldownT = cooldown.of[patternType] / patternInfos[patternType].cooldown;
		const auto patternCooldown = patternInfos[patternType].cooldown;
		const auto speed = 0.18f;
		auto& displayedCooldown = displayedCooldownTs[patternType];
		displayedCooldown = lerp(displayedCooldown, actualCooldownT, std::clamp(speed / patternCooldown, 0.0f, 1.0f));
	}

	if (selectedPatternTransition.has_value()) {
		auto& t = selectedPatternTransition->t;
		t += FRAME_DT_SECONDS * 2.0f;
		if (t <= hidingTime) {
			drawCooldownTimer(selectedPatternTransition->currentPattern, t / hidingTimeLength);
		} else if (t > showingTime) {
			drawCooldownTimer(selectedPatternTransition->newPattern, 1.0f - (t - 0.5f - betweenTransitions) / showingTimeLength);
		}

		if (t >= 1.0f) {
			selectedPatternTransition = std::nullopt;
		}
	} else {
		drawCooldownTimer(currentSelectedPattern, 0.0f);
	}
	previousFrameSelectedPattern = currentSelectedPattern;
}

void GameClient::RespawnText::updateAndRender(Renderer& renderer, bool displayed) {
	const auto speed = 2.0f * FRAME_DT_SECONDS;
	if (displayed) {
		opacity += speed;
	} else {
		opacity -= speed;
	}
	opacity = std::clamp(opacity, 0.0f, 1.0f);

	const auto textSize = 0.05f;
	const auto text = "Press space to spawn";
	const auto transform = transformRelativeToAnchor(renderer.camera, Vec2(0.0f));
	const auto textInfo = renderer.getTextInfo(renderer.font, textSize, text);
	Vec2 textPos(0.0f);
	textPos.y -= textInfo.bottomY;
	textPos -= textInfo.size / 2.0f;
	renderer.addTextToDraw(renderer.text.instances, renderer.font, textPos, transform, textSize, text, Vec4(Color3::WHITE, opacity));
}

void GameClient::drawMap(Renderer& renderer, std::unordered_map<PlayerIndex, Player> players, PlayerIndex clientPlayerIndex) {
	const auto mapAnchor = Vec2(0.5f, -0.5f);
	const auto transform = transformRelativeToAnchor(renderer.camera, mapAnchor);
	const auto size = 0.2f;
	const auto halfSize = size / 2.0f;
	
	const Vec2 direction = Vec2(-1.0f, 1.0f);
	const auto mapPos = direction * (halfSize + 0.03f);
	renderer.map.instances.toDraw.push_back(MapInstance{
		.transform = makeObjectTransform(mapPos, 0.0f, Vec2(halfSize)) * transform
	});
	const Vec2 mapCenter = mapPos;

	const auto center = renderer.camera.pos;
	const auto radius = 4.0f;
	for (const auto& [playerIndex, player] : players) {
		if (playerIndex == clientPlayerIndex) {
			continue;
		}

		if (!player.isAlive) {
			continue;
		}

		auto onMapPos = player.position - center;
		const auto length = onMapPos.length();
		const auto direction = onMapPos / length;
		onMapPos = direction * std::min(length, radius) / radius * (halfSize * 0.95f);
		const auto angle = direction.angle() + PI<float> / 2.0f;

		renderer.mapPlayerMarker.instances.toDraw.push_back(MapPlayerMarkerInstance{
			.transform = makeObjectTransform(mapCenter + onMapPos, angle, Vec2(size * 0.05f)) * transform,
			.color = Vec3(1.0f),
		});
	}
}