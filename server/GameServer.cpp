#include <GameServer.hpp>
#include <iostream>
#include <engine/Utils/RefOptional.hpp>
#include <engine/Utils/Put.hpp>
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
		// A different message needs to be allocated for every client, because if it is reliable then if it isn't acked it needs to be resent. Another option would be to use ref counted allocations I guess.
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

template<typename K, typename V>
std::optional<V&> get(std::unordered_map<K, V>& map, const K& key) {
	auto it = map.find(key);
	if (it == map.end()) {
		return std::nullopt;
	}
	return it->second;
}

GameServer::GameServer()
	: server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, yojimbo::Address("127.0.0.1", SERVER_PORT), connectionConfig, adapter, 0.0f)
	, adapter(this) {

	server.Start(MAX_CLIENTS);

	// Can only set fake network parameters after calling Start the network simulator doesn't exist before this.
	server.SetLatency(DEBUG_LATENCY);
	server.SetJitter(DEBUG_JITTER);
	server.SetPacketLoss(DEBUG_PACKET_LOSS_PERCENT);

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
void GameServer::update() {
	if (!server.IsRunning()) {
		isRunning = false;
		return;
	}
	server.AdvanceTime(server.GetTime() + 1.0f / 60.0f);
	server.ReceivePackets();

	processMessages();

	for (auto& [playerId, player] : players) {
		/*if (player.inputs.size() > maxInputsSize) {
			maxInputsSize = player.inputs.size();
			std::cout << "inputs size: " << player.inputs.size() << '\n';
		}*/

		if (player.inputs.empty()) {
			// Maybe duplicate last frame's input
		} else {
			std::cout << "inputs size" << player.inputs.size() << '\n';
			const auto [input, clientSequenceNumber] = player.inputs.front();
			player.inputs.pop();
			//player.pos = applyMovementInput(player.pos, input, dt);*/
			player.newestExecutedInputClientSequenceNumber = clientSequenceNumber;

			// Should shoot inputs be applied instantly? There would be a cooldown between shoots so there might not be an issue.
		}
	}


	if (frame % SERVER_UPDATE_SEND_RATE_DIVISOR == 0) {
		const auto sequenceNumber = frame / SERVER_UPDATE_SEND_RATE_DIVISOR;

		for (PlayerIndex clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) {
			if (!server.IsClientConnected(clientIndex)) {
				continue;
			}

			auto message = reinterpret_cast<WorldUpdateMessage*>(server.CreateMessage(clientIndex, GameMessageType::WORLD_UPDATE));
			const auto player = get(players, clientIndex);
			if (!player.has_value()) {
				CHECK_NOT_REACHED();
				continue;
			}

			if (!player->newestExecutedInputClientSequenceNumber.has_value() || !player->newestReceivedInputClientSequenceNumber.has_value()) {
				// The client needs to send a message to be able to calculate RTT.
				continue;
			}

			message->lastExecutedInputClientSequenceNumber = *player->newestExecutedInputClientSequenceNumber;
			message->lastReceivedInputClientSequenceNumber = *player->newestReceivedInputClientSequenceNumber;
			message->serverSequenceNumber = sequenceNumber;
			server.SendMessage(clientIndex, GameChannel::UNRELIABLE, message);
		}
	}

	server.SendPackets();
	put("%", frame);
	frame++;
	/*frame++;

	serverTime += FRAME_DT_MILLISECONDS;*/
	//std::cout << serverTime << '\n';
	//std::cout << server.GetTime() << '\n';
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

void GameServer::processMessage(PlayerIndex clientIndex, yojimbo::Message* message) {
	switch (message->GetType()) {
	case GameMessageType::CLIENT_INPUT: {
		const auto& msg = *reinterpret_cast<ClientInputMessage*>(message);

		auto player = get(players, clientIndex);
		if (!player.has_value()) {
			CHECK_NOT_REACHED();
			return;
		}

		const auto inputsInMessage = static_cast<int>(std::size(msg.inputs));
		int newInputs;
		if (player->newestReceivedInputClientSequenceNumber.has_value()) {
			newInputs = std::min(msg.clientSequenceNumber - static_cast<int>(*player->newestReceivedInputClientSequenceNumber), inputsInMessage);
			if (newInputs <= 0) {
				return;
			}
		} else {
			newInputs = inputsInMessage;
		}
		put("new inputs %", newInputs);
		player->newestReceivedInputClientSequenceNumber = msg.clientSequenceNumber;

		auto sequenceNumber = msg.clientSequenceNumber - newInputs + 1;
		for (auto i = inputsInMessage - newInputs; i < inputsInMessage; i++, sequenceNumber++) {
			// First message sent has empty inputs at the start. Ignore them.
			if (sequenceNumber < 0) {
				continue;
			}
			player->inputs.push(Player::InputWithSequenceNumber{
				.input = msg.inputs[i], 
				.clientSequenceNumber = static_cast<FrameTime>(sequenceNumber)
			});
		}

		break;
	}


	case GameMessageType::SPAWN_REQUEST:
		break;

	default:
		break;
	}
}

void GameServer::onClientConnected(int clientIndex) {
	std::cout << "client connected " << clientIndex << '\n';
	/*players[clientIndex] = Player{};*/
	players.insert({ clientIndex, Player{ } });
	const auto msg = reinterpret_cast<JoinMessage*>(server.CreateMessage(clientIndex, GameMessageType::JOIN));

	//msg->sentTime = frame;
	msg->clientPlayerIndex = clientIndex;
	server.SendMessage(clientIndex, GameChannel::RELIABLE, msg);
}

void GameServer::onClientDisconnected(int clientIndex) {
	std::cout << "client disconnected " << clientIndex << '\n';
}
