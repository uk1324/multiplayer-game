#include <GameServer.hpp>
#include <iostream>
#include <shared/Gameplay.hpp>

GameServer::GameServer()
	: server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, yojimbo::Address("127.0.0.1", SERVER_PORT), connectionConfig, adapter, 0.0f)
	, adapter(this) {

	server.SetLatency(1000.0f);
	server.SetJitter(50.0f);
	server.Start(MAX_CLIENTS);

	if (!server.IsRunning()) {
		//std::cout << "Could not start server on port " << serverPort << '\n';
		return;
	}

	char buffer[256];
	server.GetAddress().ToString(buffer, sizeof(buffer));
	std::cout << "Server address is " << buffer << std::endl;
	//char buffer[256];
	/*server.GetAddress().ToString(buffer, sizeof(buffer));
	std::cout << "Server started on " << buffer << '\n';*/
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
		if (player.inputs.empty()) {
			if (!player.receivedInputThisFrame) {
				// Maybe duplicate last frame's input
				std::cout << sequenceNumber << " lost input\n";
			}
		} else {
			const auto [input, sequenceNumber] = player.inputs.front();
			player.inputs.pop();
			player.pos = applyMovementInput(player.pos, input, dt);
			player.newestExecutedInputSequenceNumber = sequenceNumber;
			/*std::cout << "executing " << sequenceNumber << '\n';
			std::cout << "command buffer size" << player.inputs.size() << '\n';*/
		}
	}



	// ... process client inputs ...
    // ... update game ...
    // ... send game state to clients ..

	// https://github.com/networkprotocol/yojimbo/issues/93 - broadcast messages
	for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) {
		if (!server.IsClientConnected(clientIndex)) {
			continue;
		}

		auto message = reinterpret_cast<PlayerPositionUpdateMessage*>(
			server.CreateMessage(clientIndex, GameMessageType::PLAYER_POSITION_UPDATE)
		);
		message->set(players[clientIndex].newestExecutedInputSequenceNumber, sequenceNumber);

		//ASSERT(players.find(clientIndex) != players.end());
		const auto positionsBlockSize = sizeof(PlayerPosition) * players.size();
		//ASSERT(players[clientIndex].newestExecutedInputSequenceNumber >= 0);
		const auto positions = reinterpret_cast<PlayerPosition*>(server.AllocateBlock(clientIndex, positionsBlockSize));
		int i = 0;
		for (const auto& [playerIndex, player] : players) {
			positions[i] = PlayerPosition{ .playerIndex = playerIndex, .position = player.pos };
			i++;
		}
		server.AttachBlockToMessage(clientIndex, message, reinterpret_cast<u8*>(positions), positionsBlockSize);
		server.SendMessage(clientIndex, GameChannel::UNRELIABLE, message);
	}
	sequenceNumber++;

	server.SendPackets();
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
	//msg.inputs
	//for ()

	/*if (msg.sequenceNumber <= player.newestReceivedInputSequenceNumber) {
		return;
	}*/

	/*const auto missedInputs = msg.sequenceNumber - player.newestInputSequenceNumber - 1;
	player.newestInputSequenceNumber = msg.sequenceNumber;*/

	//player.missedInputs 
	//std::cout << msg.left << msg.right << msg.up << msg.down << '\n';
	//std::cout << "received " <<  msg.frame << '\n';
	

	/*playerIndexToLastReceivedInputFrameValue[clientIndex] = msg.frame;
	transform = applyMovementInput(transform, msg.up, msg.down, msg.left, msg.right, dt);*/
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
