#include <GameServer.hpp>
#include <iostream>
#include <shared/Gameplay.hpp>

template<typename MessageType, typename InitCallable>
void broadcastMessage(
	yojimbo::Server& server,
	GameChannel::GameChannel channel,
	GameMessageType::GameMessageType type,
	InitCallable init,
	const void* block = nullptr,
	usize blockByteSize = 0) {
	for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) {
		if (!server.IsClientConnected(clientIndex)) {
			continue;
		}
			

		auto message = reinterpret_cast<MessageType*>(server.CreateMessage(clientIndex, type));
		init(*message);
		if (block != nullptr) {
			const auto msgBlock = server.AllocateBlock(clientIndex, blockByteSize);
			std::memcpy(msgBlock, block, blockByteSize);
			server.AttachBlockToMessage(clientIndex, message, msgBlock, blockByteSize);
		}
		server.SendMessage(clientIndex, channel, message);
	}
}

GameServer::GameServer()
	: server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, yojimbo::Address("127.0.0.1", SERVER_PORT), connectionConfig, adapter, 0.0f)
	, adapter(this) {

	server.Start(MAX_CLIENTS);

	// Can only set fake network parameters after calling Start the network simulator doesn't exist before this.
	server.SetLatency(DEBUG_LATENCY);
	server.SetJitter(DEBUG_JITTER);

	if (!server.IsRunning()) {
		return;
	}

	char buffer[256];
	server.GetAddress().ToString(buffer, sizeof(buffer));
	std::cout << "Server address is " << buffer << std::endl;
}

GameServer::~GameServer() {
	server.Stop();
}

// TODO: Don't allow client to send multiple inputs per frame. Buffer the inputs and execute them on the next frame.
void GameServer::update(float dt) {
	this->dt = dt;
	if (!server.IsRunning()) {
		isRunning = false;
		return;
	}
	server.AdvanceTime(server.GetTime() + dt);
	server.ReceivePackets();

	for (auto& [_, player] : players) {
		player.receivedInputThisFrame = false;
	}
	processMessages();

	/*for (auto& [playerIndex, player] : players) {
		if (player.receivedInputThisFrame) {
			player.framesWithoutInputReceived = 0;
		} else {
			player.framesWithoutInputReceived++;
		}

		if (player.framesWithoutInputReceived > 1000) {
			server.DisconnectClient(playerIndex);
		}
	}*/

	auto assertInputsOrdered = [this]() {
		for (auto& [_, player] : players) {
			auto copy = player.inputs;
			int previous = -1;
			while (!copy.empty()) {
				ASSERT(copy.front().sequenceNumber > previous);
				copy.pop();
			}
		}
	};

	auto checkCollisions = [this]() {
		for (auto& [playerIndex, player] : players) {
			if (!player.isAlive)
				continue;

			for (const auto& [bulletIndex, bullet] : bullets) {
				if (bullet.ownerPlayerIndex == playerIndex) {
					continue;
				}

				if (distance(player.pos, bullet.pos) > BULLET_HITBOX_RADIUS + PLAYER_HITBOX_RADIUS) {
					continue;
				}

				// TODO: Send a message with all the updates at the end of the frame. It isn't very likely that multiple kills happen in one frame.

				auto& killer = players[bullet.ownerPlayerIndex];
				killer.kills++;
				auto& killed = player;
				killed.deaths++;
				killed.isAlive = false;

				const auto entryCount = 2;
				LeaderboardUpdateMessage::Entry entries[entryCount]{
					killer.leaderboardEntry(bullet.ownerPlayerIndex),
					killed.leaderboardEntry(playerIndex)
				};
				broadcastMessage<LeaderboardUpdateMessage>(
					server,
					GameChannel::RELIABLE,
					GameMessageType::LEADERBOARD_UPDATE,
					[&](LeaderboardUpdateMessage& message) {
						message.entryCount = entryCount;
					},
					&entries,
					sizeof(entries)
				);

				/*bool logged = false;
				for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) {
					if (!server.IsClientConnected(clientIndex))
						continue;
					auto update = static_cast<LeaderboardUpdateMessage*>(server.CreateMessage(clientIndex, GameMessageType::LEADERBOARD_UPDATE));
					update->entryCount = 2;
					const auto blockSize = update->entryCount * sizeof(LeaderboardUpdateMessage::Entry);
					auto block = server.AllocateBlock(clientIndex, blockSize);
					auto entries = reinterpret_cast<LeaderboardUpdateMessage::Entry*>(block);
					auto& killer = players[bullet.ownerPlayerIndex];
					killer.kills++;
					entries[0] = killer.leaderboardEntry(bullet.ownerPlayerIndex);
					auto& killed = player;
					killed.deaths++;
					killed.isAlive = false;
					entries[1] = killed.leaderboardEntry(playerIndex);

					if (!logged) {
						logged = true;
						std::cout << "{\nleaderboard update\n";
						for (int i = 0; i < update->entryCount; i++) {
							const auto entry = entries[i];
							std::cout << entry.playerIndex << ' ' << entry.kills << ' ' << entry.deaths << '\n';
						}
						std::cout << "}\n";
					}

					server.AttachBlockToMessage(clientIndex, update, block, blockSize);

					server.SendMessage(clientIndex, GameChannel::RELIABLE, update);
				}*/
				bullets.erase(bulletIndex);
				break;
			}
		}
	};

	auto processInputs = [this, dt]() {
		for (auto& [_, player] : players) {
			if (!player.isAlive) {
				player.inputs = {};
			}
		}

		for (auto& [_, player] : players) {
			player.bulletsSpawnedThisFrame = 0;
		}

		static int maxInputsSize = -1;
		for (auto& [playerId, player] : players) {

			/*if (player.inputs.size() > maxInputsSize) {
				maxInputsSize = player.inputs.size();
				std::cout << "inputs size: " << player.inputs.size() << '\n';
			}*/

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
				// Cooldowns might get desynchronized
				player.shootCooldown -= dt;
				player.shootCooldown = std::max(0.0f, player.shootCooldown);
				if (input.shoot && player.shootCooldown == 0.0f) {
					player.shootCooldown = SHOOT_COOLDOWN;
					const auto direction = Vec2::oriented(input.rotation);
					auto spawnBullet = [&](Vec2 position, Vec2 velocity) {
						bullets[bulletIndexCounter] = Bullet{
							.pos = position + PLAYER_HITBOX_RADIUS * direction,
							.velocity = velocity,
							.ownerPlayerIndex = playerId,
							.aliveFramesLeft = 1000,
							.spawnFrameClientSequenceNumber = sequenceNumber,
							.frameSpawnIndex = player.bulletsSpawnedThisFrame,
							.catchUpTime = info.RTT / 2.0f / 1000.0f + FRAME_DT * SERVER_UPDATE_SEND_RATE_DIVISOR
						};
						player.bulletsSpawnedThisFrame++;
						bulletIndexCounter++;
					};
					spawnTripleBullet(player.pos, input.rotation, BULLET_SPEED, spawnBullet);
				}
				// Should shoot inputs be applied instantly? There would be a cooldown between shoots so there might not be an issue.

			}
		}
	};

	assertInputsOrdered();
	checkCollisions();
	processInputs();

	for (auto& [_, bullet] : bullets) {
		updateBullet(bullet.pos, bullet.velocity, bullet.timeElapsed, bullet.catchUpTime, bullet.aliveFramesLeft, FRAME_DT);
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

		auto shouldSendPlayer = [](const Player& player) {
			return player.isAlive;
		};

		auto playersToSend = 0;
		for (const auto& [_, player] : players) {
			if (shouldSendPlayer(player)) {
				playersToSend++;
			}
		}
		const auto bulletsCount = bullets.size();

		if (playersToSend == 0 && bulletsCount == 0) {
			continue;
		}

		message->set(players[clientIndex].newestExecutedInputSequenceNumber, sequenceNumber, playersToSend, bulletsCount);
		
		const auto playersBlockSize = sizeof(WorldUpdateMessage::Player) * playersToSend;
		const auto bulletsBlockSize = sizeof(WorldUpdateMessage::Bullet) * bulletsCount;
		const auto blockSize = playersBlockSize + bulletsBlockSize;
		u8* block = server.AllocateBlock(clientIndex, blockSize);
		
		auto msgBullets = reinterpret_cast<WorldUpdateMessage::Bullet*>(block + playersBlockSize);

		int i;
		if (playersToSend > 0) {
			auto msgPlayers = reinterpret_cast<WorldUpdateMessage::Player*>(block);
			i = 0;
			for (const auto& [playerIndex, player] : players) {
				if (!shouldSendPlayer(player))
					continue;

				msgPlayers[i] = WorldUpdateMessage::Player{ .index = playerIndex, .position = player.pos };
				i++;
			}
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
				.timeElapsed = bullet.timeElapsed,
				.timeToCatchUp = bullet.catchUpTime,
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

	case GameMessageType::SPAWN_REQUEST:
		processSpawnRequestMessage(clientIndex, *static_cast<SpawnRequestMessage*>(message));
		break;

	default:
		break;
	}
}

void GameServer::processClientInputMessage(int clientIndex, ClientInputMessage& msg) {
	//std::cout << msg.inputs[3].left << msg.inputs[3].right << msg.inputs[3].up << msg.inputs[3].down << '\n';
	auto& player = players[clientIndex];
	if (!player.isAlive)
		return;

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

void GameServer::processSpawnRequestMessage(int clientIndex, SpawnRequestMessage& msg) {
	const auto it = players.find(clientIndex);
	if (it == players.end()) {
		ASSERT_NOT_REACHED();
		return;
	}

	auto& player = it->second;
	if (player.isAlive) {
		ASSERT_NOT_REACHED();
		return;
	}

	// TODO: Add respawn cooldown.
	player.isAlive = true;
	for (int clientI = 0; clientI < MAX_CLIENTS; clientI++) {
		if (!server.IsClientConnected(clientI))
			continue;

		auto message = static_cast<SpawnMessage*>(server.CreateMessage(clientI, GameMessageType::SPAWN_PLAYER));
		message->playerIndex = clientIndex;
		server.SendMessage(clientI, GameChannel::RELIABLE, message);
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

LeaderboardUpdateMessage::Entry GameServer::Player::leaderboardEntry(i32 playerIndex) const {
	return LeaderboardUpdateMessage::Entry{
		.playerIndex = playerIndex,
		.deaths = deaths,
		.kills = kills,
	};
}
