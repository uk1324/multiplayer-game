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

#include <queue>

std::queue<int> delaysExecuted;
std::queue<int> delaysReceived;
static int d = 0;

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


	/*serverTime += FRAME_DT_MILLISECONDS;
	std::cout << serverTime << '\n';*/
	/*std::cout << serverTime << '\n';
	serverTime++;*/
	//std::cout << client.GetTime() << '\n';
	auto avgD = [](const std::queue<int>& frames) {
		float s = 0.0f;
		for (auto v : frames._Get_container()) {
			s += v;
		}
		return static_cast<int>(s /= frames.size());
	};

	sequenceNumber++;
	frame++;
	yojimbo::NetworkInfo info;
	client.GetNetworkInfo(info);
	const auto avgDExecuted = avgD(delaysExecuted);
	const auto avgDReceived = avgD(delaysReceived);
	put("executed: % received: % difference: % RTT: % executed delay: % received delay: %", sequenceNumber + avgDExecuted, sequenceNumber + avgDReceived, avgDExecuted - avgDReceived, info.RTT, avgDExecuted, avgDReceived);
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
			
			/*put("%", sequenceNumber - msg.lastExecutedInputClientSequenceNumber);*/
			/*put("% % %", sequenceNumber - msg.lastExecutedInputClientSequenceNumber, sequenceNumber, msg.lastExecutedInputClientSequenceNumber);*/
			const auto d2 = sequenceNumber - msg.lastExecutedInputClientSequenceNumber;
			//const auto d = (sequenceNumber - msg.serverSequenceNumber * SERVER_UPDATE_SEND_RATE_DIVISOR);
			yojimbo::NetworkInfo info;
			client.GetNetworkInfo(info);
			/*const auto differenceBetweenClocks = (msg.lastExecutedInputClientSequenceNumber - msg.serverSequenceNumber * SERVER_UPDATE_SEND_RATE_DIVISOR) * 2.0f;
			std::cout << sequenceNumber + differenceBetweenClocks << '\n';*/
			const auto serverFrame = msg.serverSequenceNumber * SERVER_UPDATE_SEND_RATE_DIVISOR;

			auto addDelay = [](std::queue<int>& delays, int d) {
				delays.push(d);
				if (delays.size() > 10) {
					delays.pop();
				}
			};
			// https://en.wikipedia.org/wiki/Network_Time_Protocol#Clock_synchronization_algorithm
			// Average of the time it takes for the packet to get to the server. The first difference is the time it takes to get to the server the second is how long it takes to get back from the server.
			int dExecuted = ((serverFrame - msg.lastExecutedInputClientSequenceNumber) + (serverFrame - sequenceNumber)) / 2.0f;
			int dReceived = ((serverFrame - msg.lastReceivedInputClientSequenceNumber) + (serverFrame - sequenceNumber)) / 2.0f;
			addDelay(delaysExecuted, dExecuted);
			addDelay(delaysReceived, dReceived);
			
			//const auto rtt = (sequenceNumber - msg);
			//put("%", d);
			//const auto differenceBetweenClocks = (sequenceNumber - msg.serverSequenceNumber * SERVER_UPDATE_SEND_RATE_DIVISOR) * 2.0f;
			//frame = sequenceNumber - differenceBetweenClocks;
			//std::cout << frame << '\n';
			/*put("% % % % %", d, d * (1.0f / 60.0f), d2, d2 * (1.0f / 60.0f), info.RTT);*/
			//put("desynch between clocks: %\nRTT: %", d * (1.0f / 60.0f), info.RTT / 1000.0f);
			//put("%", d);
			//frame -= d;
			/*yojimbo::NetworkInfo info;
			client.GetNetworkInfo(info);
			if (msg.sentTime < serverTime) {
				put("smaller %", info.RTT);
			} else if (serverTime - msg.sentTime > 2) {
				put("bigger %", info.RTT);
			}
			serverTime = msg.sentTime;*/

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