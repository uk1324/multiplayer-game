#include <GameServer.hpp>
#include <iostream>
#include <shared/Gameplay.hpp>

GameServer::GameServer()
	: server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, yojimbo::Address("127.0.0.1", SERVER_PORT), connectionConfig, adapter, 0.0f)
	, adapter(this) {

	server.SetLatency(DEBUG_LATENCY);
	server.SetJitter(DEBUG_JITTER);
	server.Start(MAX_CLIENTS);

	if (!server.IsRunning()) {
		return;
	}

	char buffer[256];
	server.GetAddress().ToString(buffer, sizeof(buffer));
	std::cout << "Server address is " << buffer << std::endl;
}

// TODO: Don't allow client to send multiple inputs per frame. Buffer the inputs and execute them on the next frame.
void GameServer::update(float dt) {
	this->dt = dt;
	if (!server.IsRunning()) {
		isRunning = false;
		return;
	}

	server.AdvanceTime(dt);
	server.ReceivePackets();

	for (auto& [_, player] : players) {
		player.receivedInputThisFrame = false;
	}
	processMessages();

	for (auto& [playerIndex, player] : players) {
		if (player.receivedInputThisFrame) {
			player.framesWithoutInputReceived = 0;
		} else {
			player.framesWithoutInputReceived++;
		}

		if (player.framesWithoutInputReceived > 1000) {
			server.DisconnectClient(playerIndex);
		}
	}

	for (auto& [_, player] : players) {
		auto copy = player.inputs;
		int previous = -1;
		while (!copy.empty()) {
			ASSERT(copy.front().sequenceNumber > previous);
			copy.pop();
		}
	}

	for (auto& [_, player] : players) {
		player.bulletsSpawnedThisFrame = 0;
	}

	for (const auto& [playerIndex, player] : players) {
		for (const auto& [_, bullet] : bullets) {
			if (bullet.ownerPlayerIndex == playerIndex) {
				continue;
			}

			if (distance(player.pos, bullet.pos) < BULLET_HITBOX_RADIUS + PLAYER_HITBOX_RADIUS) {
				auto message = server.CreateMessage(playerIndex, GameMessageType::TEST);
				server.SendMessage(playerIndex, GameChannel::RELIABLE, message);
			}
		}
	}

	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (server.IsClientConnected(i)) {
			yojimbo::NetworkInfo info;
			server.GetNetworkInfo(i, info);
		}
	}

	static int maxInputsSize = -1;
	for (auto& [playerId, player] : players) {
		
		if (player.inputs.size() > maxInputsSize) {
			maxInputsSize = player.inputs.size();
			std::cout << "inputs size: " << player.inputs.size() << '\n';
		}
		if (player.inputs.empty()) {
			if (!player.receivedInputThisFrame) {
				// Maybe duplicate last frame's input
				//std::cout << sequenceNumber << " lost input\n";
			}
		} else {
			const auto [input, sequenceNumber] = player.inputs.front();
			player.inputs.pop();
			player.pos = applyMovementInput(player.pos, input, dt);
			player.newestExecutedInputSequenceNumber = sequenceNumber;

			
			yojimbo::NetworkInfo info;
			server.GetNetworkInfo(playerId, info);

			if (input.shoot) {
				const auto direction = Vec2::oriented(input.rotation);
				std::cout << info.RTT << '\n'; // For some reason RTT is broken and is always zero so its using DEBUG_LATENCY instead.
				bullets[bulletIndexCounter] = Bullet{
					.pos = player.pos + PLAYER_HITBOX_RADIUS * direction + ((DEBUG_LATENCY) / 1000.0f + 3 * FRAME_DT) * direction,
					.velocity = direction,
					.ownerPlayerIndex = playerId,
					.aliveFramesLeft = 1000,
					.spawnFrameClientSequenceNumber = sequenceNumber,
					.frameSpawnIndex = player.bulletsSpawnedThisFrame
				};
				player.bulletsSpawnedThisFrame++;
				bulletIndexCounter++;
			}
			// Should shoot inputs be applied instantly? There would be a cooldown between shoots so there might not be an issue.
			
		}
	}

	for (auto& [_, bullet] : bullets) {
		bullet.pos += bullet.velocity * dt;
		bullet.aliveFramesLeft--;
	}
	std::erase_if(bullets, [](const auto& item) { return item.second.aliveFramesLeft <= 0; });
	

	if (frame % SERVER_UPDATE_SEND_RATE_DIVISOR == 0) {
		broadcastWorldState();
	}

	server.SendPackets();
	frame++;
}

void GameServer::broadcastWorldState() {
	// https://github.com/networkprotocol/yojimbo/issues/93 - broadcast messages

	for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) {
		if (!server.IsClientConnected(clientIndex)) {
			continue;
		}

		auto message = reinterpret_cast<WorldUpdateMessage*>(server.CreateMessage(clientIndex, GameMessageType::WORLD_UPDATE));

		const auto playersCount = players.size();
		const auto bulletsCount = bullets.size();
		message->set(players[clientIndex].newestExecutedInputSequenceNumber, sequenceNumber, playersCount, bulletsCount);

		const auto playersBlockSize = sizeof(WorldUpdateMessage::Player) * playersCount;
		const auto bulletsBlockSize = sizeof(WorldUpdateMessage::Bullet) * bulletsCount;
		const auto blockSize = playersBlockSize + bulletsBlockSize;
		u8* block = server.AllocateBlock(clientIndex, blockSize);
		auto msgPlayers = reinterpret_cast<WorldUpdateMessage::Player*>(block);
		auto msgBullets = reinterpret_cast<WorldUpdateMessage::Bullet*>(block + playersBlockSize);

		int i;
		i = 0;
		for (const auto& [playerIndex, player] : players) {
			msgPlayers[i] = WorldUpdateMessage::Player{ .index = playerIndex, .position = player.pos };
			i++;
		}
		i = 0;
		for (const auto& [bulletIndex, bullet] : bullets) {
			msgBullets[i] = WorldUpdateMessage::Bullet{ 
				.index = bulletIndex, 
				.position = bullet.pos, 
				.velocity = bullet.velocity,
				.ownerPlayerIndex = bullet.ownerPlayerIndex,
				.aliveFramesLeft = bullet.aliveFramesLeft,
				.spawnFrameClientSequenceNumber = bullet.spawnFrameClientSequenceNumber,
				.frameSpawnIndex = bullet.frameSpawnIndex,
			};
			i++;
		}

		server.AttachBlockToMessage(clientIndex, message, block, blockSize);
		server.SendMessage(clientIndex, GameChannel::UNRELIABLE, message);
	}
	sequenceNumber++;
}

void GameServer::processMessages() {
	for (int clientI = 0; clientI < MAX_CLIENTS; clientI++) {
		if (server.IsClientConnected(clientI)) {
			for (int channelI = 0; channelI < connectionConfig.numChannels; channelI++) {
				yojimbo::Message* msg;
				for (;;) {
					msg = server.ReceiveMessage(clientI, channelI);
					if (msg == nullptr)
						break;
					processMessage(clientI, msg);
					server.ReleaseMessage(clientI, msg);
				}
			}
		}
	}
}

void GameServer::processMessage(int clientIndex, yojimbo::Message* message) {
	switch (message->GetType()) {
	case GameMessageType::CLIENT_INPUT:
		processClientInputMessage(clientIndex, *static_cast<ClientInputMessage*>(message));
		break;
	}
}

void GameServer::processClientInputMessage(int clientIndex, ClientInputMessage& msg) {
	//std::cout << msg.inputs[3].left << msg.inputs[3].right << msg.inputs[3].up << msg.inputs[3].down << '\n';
	auto& player = players[clientIndex];
	const auto inputsInMessage = static_cast<int>(std::size(msg.inputs));
	const auto newInputs = std::min(msg.sequenceNumber - player.newestReceivedInputSequenceNumber, inputsInMessage);
	if (newInputs <= 0) {
		return;
	}
	player.newestReceivedInputSequenceNumber = msg.sequenceNumber;
	player.receivedInputThisFrame = true;

	int sequenceNumber = msg.sequenceNumber - newInputs + 1;
	for (int i = inputsInMessage - newInputs; i < inputsInMessage; i++, sequenceNumber++) {
		// First message sent has empty inputs at the start. Ignore them.
		if (sequenceNumber < 0) {
			continue;
		}
		player.inputs.push(Player::InputWithSequenceNumber{ .input = msg.inputs[i], .sequenceNumber = sequenceNumber });
	}
}

void GameServer::onClientConnected(int clientIndex) {
	std::cout << "client connected " << clientIndex << '\n';
	players[clientIndex] = Player{};
	const auto msg = reinterpret_cast<JoinMessage*>(server.CreateMessage(clientIndex, GameMessageType::JOIN));
	msg->clientPlayerIndex = clientIndex;
	msg->currentFrame = -1;
	server.SendMessage(clientIndex, GameChannel::RELIABLE, msg);
}

void GameServer::onClientDisconnected(int clientIndex) {
	std::cout << "client disconnected " << clientIndex << '\n';
}
