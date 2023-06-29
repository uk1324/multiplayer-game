#include <client/MainLoop.hpp>
#include <imgui/imgui.h>

MainLoop::MainLoop()
	: client(yojimbo::GetDefaultAllocator(), yojimbo::Address("0.0.0.0"), connectionConfig, adapter, 0.0)
	, game(client, renderer) {
	//connect(yojimbo::Address("127.0.0.1", SERVER_PORT));
}

#include <Gui.hpp>
#include <engine/Json/JsonPrinter.hpp>
#include <client/Debug.hpp>

void MainLoop::update() {
	client.AdvanceTime(client.GetTime() + FRAME_DT);
	client.ReceivePackets();
	if (client.IsConnected()) {
		processMessages();
		if (game.joinedGame()) {
			game.update();
		}
	} else {
		ImGui::Begin("game menu");

		static char input[20] = "127.0.0.1";

		{
			ImGui::BeginDisabled(client.IsConnecting());
			ImGui::InputText("address", input, sizeof(input));
			ImGui::EndDisabled();
		}

		if (!client.IsConnecting()) {
			if (ImGui::Button("connect")) {
				connect(yojimbo::Address(input, SERVER_PORT));
			}
		} else {
			if (ImGui::Button("stop connecting")) {
				client.Disconnect();
			}
		}

		if (client.ConnectionFailed()) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			ImGui::Text("failed to connect");
			ImGui::PopStyleColor();
		}

		ImGui::End();
	}
	renderer.update();
	client.SendPackets(); // Could send packets eailer, maybe right after processing input or after render.
}

void MainLoop::processMessages() {
	for (int i = 0; i < connectionConfig.numChannels; i++) {
		yojimbo::Message* message = client.ReceiveMessage(i);
		while (message != nullptr) {
			processMessage(message);
			client.ReleaseMessage(message);
			message = client.ReceiveMessage(i);
		}
	}
}

void MainLoop::processMessage(yojimbo::Message* message) {
	if (client.IsConnected() && game.joinedGame()) {
		game.processMessage(message);
	}

	switch (message->GetType()) {
		case GameMessageType::JOIN: {
			if (game.joinedGame()) {
				CHECK_NOT_REACHED();
				break;
			} 
			const auto msg = static_cast<JoinMessage*>(message);
			game.onJoin(msg->clientPlayerIndex);
			break;
		}
	default:
		break;
	}
}

void MainLoop::connect(const yojimbo::Address& address) {
	if (!address.IsValid()) {
		return;
	}
	u64 clientId;
	yojimbo::random_bytes((uint8_t*)&clientId, 8);
	client.InsecureConnect(DEFAULT_PRIVATE_KEY, clientId, address);
	client.SetLatency(DEBUG_LATENCY);
	client.SetJitter(DEBUG_JITTER);
}
