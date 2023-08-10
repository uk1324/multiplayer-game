#include <GameServer.hpp>
#include <iostream>
#include <engine/Utils/RefOptional.hpp>
#include <engine/Utils/Put.hpp>
#include <engine/Utils/MapOptGet.hpp>
#include <engine/Math/Random.hpp>
#include <server/ServerGameplayContext.hpp>
#include <shared/DebugSettings.hpp>
#include <random>
#include <yojimbo/netcode.io/netcode.h>

// TODO: Idea for sending only spawned objects. 
// With each object on the server store 2 bitsets of size max players. 
// One for if the object was sent an another for if it was acked.
// The issue with this is that there is a RTT of delay between send and acking. Should the server send data during that time. I guess there wouldn't be an issue with that.

// Only send a message if an update was not received. 
// How would a client know if an update was not received.
// Checking sequence numbers isn't perfect because of out of order updates, but might work.


// Between updates store the spawned object indices.
// Store past updates.
// If update not acked resend.
// This is less flexible, because each player o receives the same objects even if they don't need them.

template<typename MessageType, typename InitCallable>
static void broadcastMessage(
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
			memcpy(msgBlock, block, blockByteSize);
			server.AttachBlockToMessage(clientIndex, message, msgBlock, blockByteSize);
		}
		server.SendMessage(clientIndex, channel, message);
	}
}

GameServer::GameServer(const char* address)
	: server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, yojimbo::Address(address, SERVER_PORT), connectionConfig, adapter, 0.0f)
	, adapter(this)
	, httpServerThread(&HttpServer::run, &httpServer)
	#ifdef DEBUG_REPLAY_RECORDER
	, replayRecorder("./generated/serverReplay.json") 
	#endif
{

	server.Start(MAX_CLIENTS);

	#ifdef DEBUG_NETWORK_SIMULATOR
	// Can only set fake network parameters after calling Start the network simulator doesn't exist before this.
	put("using network simulator");
	server.SetLatency(DEBUG_NETOWRK_SIMULATOR_LATENCY);
	server.SetJitter(DEBUG_NETOWRK_SIMULATOR_JITTER);
	server.SetPacketLoss(DEBUG_NETOWRK_SIMULATOR_PACKET_LOSS_PERCENT);
	#endif

	if (!server.IsRunning()) {
		return;
	}

	char buffer[256];
	server.GetAddress().ToString(buffer, sizeof(buffer));
	std::cout << "Server address is " << buffer << std::endl;
	put("max players = %", MAX_CLIENTS);
}

GameServer::~GameServer() {
	server.Stop();
}

// TODO: Don't allow client to send multiple inputs per frame. Buffer the inputs and execute them on the next frame.
void GameServer::update() {
	communicateWithHttpServer();

	if (!server.IsRunning()) {
		isRunning = false;
		return;
	}
	#ifdef DEBUG_REPLAY_RECORDER
	if (replayRecorder.isRecording) {
		gameplayPlayers.clear();
		for (const auto& [_, player] : players) {
			gameplayPlayers.push_back(player.gameplayPlayer);
		}
		replayRecorder.addFrame(gameplayPlayers, gameplayState);
	}
	#endif

	server.AdvanceTime(server.GetTime() + 1.0f / 60.0f);
	server.ReceivePackets();

	processMessages();

	updateGemeplayStateBeforeProcessingInput(gameplayState);
	for (auto& [playerIndex, player] : players) {
		/*if (player.inputs.size() > maxInputsSize) {
			maxInputsSize = player.inputs.size();
			std::cout << "inputs size: " << player.inputs.size() << '\n';
		}*/

		if (player.inputs.empty()) {
			// Maybe duplicate last frame's input
		} else {
			//std::cout << "inputs size" << player.inputs.size() << '\n';
			const auto& [input, clientSequenceNumber] = player.inputs.front();
			player.inputs.pop();
			//player.pos = applyMovementInput(player.pos, input, dt);*/
			player.newestExecutedInputClientSequenceNumber = clientSequenceNumber;

			// Should shoot inputs be applied instantly? There would be a cooldown between shoots so there might not be an issue.
			if (player.isAlive) {
				updateGameplayPlayer(playerIndex, player.gameplayPlayer, gameplayState, input, clientSequenceNumber, FRAME_DT_SECONDS);
			}
		}
	}	
	ServerGameplayContext context(*this);
	updateGameplayStateAfterProcessingInput(gameplayState, context, FRAME_DT_SECONDS);

	for (auto& [playerIndex, player] : players) {
		if (!player.isAlive)
			continue;

		for (const auto& [bulletIndex, bullet] : gameplayState.moveForwardBullets) {
			if (bulletIndex.ownerPlayerIndex == playerIndex) {
				continue;
			}

			const auto collided = (player.gameplayPlayer.position - bullet.position).lengthSq() < pow(BULLET_HITBOX_RADIUS + PLAYER_HITBOX_RADIUS, 2.0f);
			if (!collided) {
				continue;
			}

			auto owner = get(players, bulletIndex.ownerPlayerIndex);
			if (!owner.has_value()) {
				CHECK_NOT_REACHED();
				continue;
			}
			owner->leaderboard.kills++;
			player.leaderboard.deaths++;
			player.isAlive = false;
			broadcastMessage<LeaderboardUpdateMessage>(
				server,
				GameChannel::RELIABLE,
				GameMessageType::LEADERBOARD_UPDATE,
				[&](LeaderboardUpdateMessage& message) {
					message.entries.push_back({ bulletIndex.ownerPlayerIndex, owner->leaderboard });
					message.entries.push_back({ playerIndex, player.leaderboard });
				}
			);
		}
	}

	if (frame % SERVER_UPDATE_SEND_RATE_DIVISOR == 0) {
		broadcastWorldState();
	}

	server.SendPackets();
	//put("%", frame);
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

void GameServer::processMessage(int clientIndex, yojimbo::Message* message) {
	const auto playerIndex = get(clientIndexToPlayerIndex, clientIndex);
	if (!playerIndex.has_value()) {
		CHECK_NOT_REACHED();
		return;
	}

	switch (message->GetType()) {
	case GameMessageType::CLIENT_INPUT: {
		const auto& msg = *reinterpret_cast<ClientInputMessage*>(message);

		

		auto player = get(players, *playerIndex);
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
		//put("new inputs %", newInputs);
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

	case GameMessageType::SPAWN_REQUEST: {
		put("received spawn request from clientIndex = % playerIndex = %", clientIndex, playerIndex->value);
		auto player = get(players, *playerIndex);
		if (!player.has_value()) {
			CHECK_NOT_REACHED();
			return;
		}
		player->isAlive = true;
		player->gameplayPlayer.position = randomPointInUnitCircle() * (BORDER_RADIUS - PLAYER_HITBOX_RADIUS);
		broadcastMessage<SpawnPlayerMessage>(
			server,
			GameChannel::RELIABLE,
			GameMessageType::SPAWN_PLAYER,
			[&](SpawnPlayerMessage& msg) {
				msg.playerIndex = *playerIndex;
			}
		);
		break;
	}

	default:
		break;
	}
}

void GameServer::onClientConnected(int clientIndex) {
	netcode_address_t* address = server.GetClientAddress(clientIndex);
	// For some reason only stored in the .c file
	const auto NETCODE_MAX_ADDRESS_STRING_LENGTH = 256;
	char addressString[NETCODE_MAX_ADDRESS_STRING_LENGTH];
	netcode_address_to_string(address, addressString);

	put("client connected clientIndex = %, address = %", clientIndex, addressString);
	const Player player{
		.gameplayPlayer = {
			.position = Vec2(0.0f)
		}
	};

	const auto playerIndex = nextPlayerIndex;
	players.insert({ playerIndex, player });
	clientIndexToPlayerIndex.insert({ clientIndex, playerIndex });
	put("playerIndex = %", playerIndex);
	nextPlayerIndex.value++;

	auto joinMessagePlayerFromPlayer = [](PlayerIndex playerIndex, const Player& player) {
		return JoinMessagePlayer{
			.playerIndex = playerIndex,
			.isAlive = player.isAlive,
			.leaderboard = player.leaderboard,
		};
	};

	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (i == clientIndex) {
			continue;
		}

		if (!server.IsClientConnected(i)) {
			continue;
		}
		auto message = reinterpret_cast<PlayerJoinedMessage*>(server.CreateMessage(i, GameMessageType::PLAYER_JOINED));
		message->player = joinMessagePlayerFromPlayer(playerIndex, player);
		server.SendMessage(i, GameChannel::RELIABLE, message);
	}

	const auto msg = reinterpret_cast<JoinMessage*>(server.CreateMessage(clientIndex, GameMessageType::JOIN));
	for (const auto& [playerIndex, player] : players) {
		msg->players.push_back(joinMessagePlayerFromPlayer(playerIndex, player));
	}

	msg->clientPlayerIndex = playerIndex;
	server.SendMessage(clientIndex, GameChannel::RELIABLE, msg);
}

void GameServer::onClientDisconnected(int clientIndex) {
	std::cout << "client disconnected " << clientIndex << '\n';
	const auto playerIndex = get(clientIndexToPlayerIndex, clientIndex);
	if (!playerIndex.has_value()) {
		CHECK_NOT_REACHED();
		return;
	}

	broadcastMessage<PlayerDisconnectedMessage>(
		server,
		GameChannel::RELIABLE,
		GameMessageType::PLAYER_DISCONNECTED,
		[&](PlayerDisconnectedMessage& msg) {
			msg.playerIndex = *playerIndex;
		}
	);

	players.erase(*playerIndex);
	clientIndexToPlayerIndex.erase(clientIndex);
}

void GameServer::broadcastWorldState() {
	const auto sequenceNumber = frame / SERVER_UPDATE_SEND_RATE_DIVISOR;

	// Maybe iterate over clientIndexToPlayerIndex instead.
	for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) {
		if (!server.IsClientConnected(clientIndex)) {
			continue;
		}

		const auto playerIndex = get(clientIndexToPlayerIndex, clientIndex);

		const auto player = get(players, *playerIndex);
		if (!player.has_value()) {
			CHECK_NOT_REACHED();
			continue;
		}

		if (!player->newestExecutedInputClientSequenceNumber.has_value() || !player->newestReceivedInputClientSequenceNumber.has_value()) {
			// The client needs to send a message to be able to calculate RTT.
			continue;
		}
		auto message = reinterpret_cast<WorldUpdateMessage*>(server.CreateMessage(clientIndex, GameMessageType::WORLD_UPDATE));
		message->lastExecutedInputClientSequenceNumber = *player->newestExecutedInputClientSequenceNumber;
		message->lastReceivedInputClientSequenceNumber = *player->newestReceivedInputClientSequenceNumber;
		message->serverSequenceNumber = sequenceNumber;
		for (const auto& [playerIndex, player] : players) {
			if (!player.isAlive) {
				continue;
			}
				
			message->players.push_back(WorldUpdateMessagePlayer{
				.playerIndex = playerIndex,
				.position = player.gameplayPlayer.position,
			});
		}
		// TODO: Don't copy the whole state. Could just pass a reference (both on the client and server). For the server could add an additional counter to make sure that the data hasn't been modified from the creation time to serialization time. This shouldn't be a problem I think, because this is an unreliable message. If needed could use shared pointers. On the clinet there would just be 2 states one for loading which would be reset before deserizalizing and another for actual use.
		message->gemeplayState = gameplayState;
		server.SendMessage(clientIndex, GameChannel::UNRELIABLE, message);
	}
}

void GameServer::communicateWithHttpServer() {
	{
		auto lock = httpServer.messages.lock();
		const auto& messagesString = lock->string();

		struct Result {
			std::string_view rest;
			std::string_view message;
		};
		auto getNextMessage = [](std::string_view messages) -> std::string_view {

		};

		std::string_view messages(messagesString);
		for (int i = 0; i < messages.size(); i++) {
			if (i == 0 || (messages[i] == '\0' && i != messages.size() - 1)) {
				std::cout << "[HTTP server] ";
			}
			std::cout << messages[i];
		}
		lock->string().clear();
	}

	{
		auto lock = httpServer.players.tryLock();
		if (!lock.has_value()) {
			return;
		}
		auto& httpServerPlayers = **lock;
		httpServerPlayers.clear();

		for (const auto& [clientIndex, playerIndex] : clientIndexToPlayerIndex) {
			yojimbo::NetworkInfo info;
			server.GetNetworkInfo(clientIndex, info);
			httpServerPlayers.push_back(RequestPlayer{
				.playerIndex = static_cast<i32>(playerIndex.value),
				.rttSeconds = info.RTT / 1000.0f,
				.packetLossPercent = info.packetLoss,
			});
		}
	}
}
