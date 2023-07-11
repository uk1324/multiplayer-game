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

	const auto cursorPosWorldSpace = Input::cursorPosClipSpace() * renderer.camera.clipSpaceToWorldSpace();
	//const auto cursorRelativeToPlayer = cursorPosWorldSpace - playerTransform.position;
	//const auto rotation = atan2(cursorRelativeToPlayer.y, cursorRelativeToPlayer.x);
	float rotation = 0.0f;
	const auto newInput = ClientInputMessage::Input{
		.up = Input::isKeyHeld(KeyCode::W),
		.down = Input::isKeyHeld(KeyCode::S),
		.left = Input::isKeyHeld(KeyCode::A),
		.right = Input::isKeyHeld(KeyCode::D),
		.shoot = Input::isMouseButtonHeld(MouseButton::LEFT),
		.shift = Input::isKeyHeld(KeyCode::LEFT_SHIFT),
		.rotation = rotation
	};
	//pastInputCommands.push_back(newInput);

	//const auto oldCommandsToDiscardCount = static_cast<int>(pastInputCommands.size()) - ClientInputMessage::INPUTS_COUNT;
	/*if (oldCommandsToDiscardCount > 0) {
		pastInputCommands.erase(pastInputCommands.begin(), pastInputCommands.begin() + oldCommandsToDiscardCount);
	}*/
	//std::cout << pastInputCommands.size() << '\n';

	const auto inputMsg = static_cast<ClientInputMessage*>(client.CreateMessage(GameMessageType::CLIENT_INPUT));
	inputMsg->clientSequenceNumber = sequenceNumber;
	client.SendMessage(GameChannel::UNRELIABLE, inputMsg);

	sequenceNumber++;
	frame++;
	yojimbo::NetworkInfo info;
	client.GetNetworkInfo(info);
	
	//put("executed: % received: % difference: % RTT: % executed delay: % received delay: %", sequenceNumber + executedDelay, sequenceNumber + receivedDelay, executedDelay - receivedDelay, info.RTT, executedDelay, receivedDelay);
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

			// https://en.wikipedia.org/wiki/Network_Time_Protocol#Clock_synchronization_algorithm
			// Average of the time it takes for the packet to get to the server. The first difference is the time it takes to get to the server the second is how long it takes to get back from the server.
			const auto delayExecuted = ((serverFrame - msg.lastExecutedInputClientSequenceNumber) + (serverFrame - sequenceNumber)) / 2.0f; // Should this be using sequence number or something else?
			const auto delayReceived = ((serverFrame - msg.lastReceivedInputClientSequenceNumber) + (serverFrame - sequenceNumber)) / 2.0f;
			addDelay(pastReceivedDelays, delayReceived);
			receivedDelay = averageDelay(pastReceivedDelays);
			addDelay(pastExecutedDelays, delayExecuted);
			executedDelay = averageDelay(pastExecutedDelays);
			// NOTE: The delay between the server and the client seems to grow don't know why.
	
			static int f = 0;
			// Maybe just wait till you get the RTT and only then start displaying.
			if (f < 50) {
				yojimbo::NetworkInfo info;
				client.GetNetworkInfo(info);
				const auto rtt = sequenceNumber - msg.lastReceivedInputClientSequenceNumber;

				put("% % %", info.RTT / 1000.0f, rtt * (1.0f / 60.0f), receivedDelay);
				f++;
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

void GameClient::addDelay(std::vector<FrameTime>& delays, FrameTime newDelay) {
	delays.insert(delays.begin(), newDelay);
	if (delays.size() > 10) {
		delays.pop_back();
	}
}

FrameTime GameClient::averageDelay(const std::vector<FrameTime>& delays) {
	// TODO: Technically you could calculate the delays on the fly and not store the values by multiplying by the previous count removing the last time and adding the new one and the dividing by the new count. This wouldn't be fully correct because of rounding errors in floats.
	float s = 0.0f;
	for (const auto& v : delays) {
		s += v;
	}
	return static_cast<FrameTime>(floor(s / static_cast<float>(delays.size())));
}
