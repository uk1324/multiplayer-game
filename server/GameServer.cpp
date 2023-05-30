#include <GameServer.hpp>
#include <iostream>
#include <shared/Gameplay.hpp>

GameServer::GameServer()
	: server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, yojimbo::Address("127.0.0.1", SERVER_PORT), connectionConfig, adapter, 0.0f)
	, adapter(this) {

	server.SetLatency(1000.0f);
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

void GameServer::update(float dt) {
	this->dt = dt;
	if (!server.IsRunning()) {
		isRunning = false;
		return;
	}

	server.AdvanceTime(dt);
	server.ReceivePackets();
	processMessages();

	// ... process client inputs ...
    // ... update game ...
    // ... send game state to clients ..

	// https://github.com/networkprotocol/yojimbo/issues/93 - broadcast messages
	if (currentFrame % 6 == 0) {
		for (int clientI = 0; clientI < MAX_CLIENTS; clientI++) {
			if (!server.IsClientConnected(clientI)) {
				continue;
			}
			/*std::cout << "sending message to " << clientI << '\n';
			const auto msg = server.CreateMessage(clientI, static_cast<int>(GameMessageType::CLIENT_INPUT));
			server.SendMessage(clientI, GameChannel::RELIABLE, msg);*/
			auto message = reinterpret_cast<PlayerPositionUpdateMessage*>(
				server.CreateMessage(clientI, GameMessageType::PLAYER_POSITION_UPDATE)
				);
			message->lastReceivedInputFrameValue = playerIndexToLastReceivedInputFrameValue[clientI];
			const auto positionsBlockSize = sizeof(PlayerPosition) * playerIndexToTransform.size();
			const auto positions = reinterpret_cast<PlayerPosition*>(server.AllocateBlock(clientI, positionsBlockSize));
			int i = 0;
			for (const auto& [playerIndex, position] : playerIndexToTransform) {
				positions[i] = PlayerPosition{ .playerIndex = playerIndex, .position = position };
				i++;
			}

			server.AttachBlockToMessage(clientI, message, reinterpret_cast<u8*>(positions), positionsBlockSize);
			server.SendMessage(clientI, GameChannel::UNRELIABLE, message);
		}
	}

	server.SendPackets();
	currentFrame++;
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
	//std::cout << msg.left << msg.right << msg.up << msg.down << '\n';
	auto& transform = playerIndexToTransform[clientIndex];
	std::cout << "received " <<  msg.frame << '\n';
	playerIndexToLastReceivedInputFrameValue[clientIndex] = msg.frame;
	transform = applyMovementInput(transform, msg.up, msg.down, msg.left, msg.right, dt);
}

void GameServer::onClientConnected(int clientIndex) {
	std::cout << "client connected " << clientIndex << '\n';
	playerIndexToTransform[clientIndex] = Vec2{ 0.0f };
	const auto msg = reinterpret_cast<JoinMessage*>(server.CreateMessage(clientIndex, GameMessageType::JOIN));
	msg->clientPlayerIndex = clientIndex;
	msg->currentFrame = currentFrame;
	server.SendMessage(clientIndex, GameChannel::RELIABLE, msg);
}

void GameServer::onClientDisconnected(int clientIndex) {
	std::cout << "client disconnected " << clientIndex << '\n';
}
