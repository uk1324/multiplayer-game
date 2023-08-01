#include <client/MainLoop.hpp>
#include <imgui/imgui.h>
#include <engine/Window.hpp>
#include <engine/Input/Input.hpp>
#include <engine/Utils/Put.hpp>
#include <shared/DebugSettings.hpp>

MainLoop::MainLoop()
	: client(yojimbo::GetDefaultAllocator(), yojimbo::Address("0.0.0.0"), connectionConfig, adapter, 0.0)
	, adapter(*this)
	, game(client, renderer) {
	//connect(yojimbo::Address("127.0.0.1", SERVER_PORT));
	//Window::enableWindowedFullscreen();
}

/*
C++ doesn't seperate object allocation and intialization. This is probably because of destructors, which require the object to be in a correct state at all times.

Implementing a reset function that reinitializes the object into it's initial state without reallocating is very error prone.

One way to handle this could be to move the allocated objects from the old instance into the new instance. Which lets you choose which objects to take from the old instance, but still doesn't solve the problem of resetting the state of the taken members. The only full soultion might be generating the reset function using a code generator.
*/
MainLoop::~MainLoop() {
	client.Disconnect();
}

#include <Gui.hpp>
#include <engine/Json/JsonPrinter.hpp>
#include <client/Debug.hpp>

void MainLoop::update() {
	client.AdvanceTime(client.GetTime() + FRAME_DT_SECONDS);
	client.ReceivePackets();

	/*if (Input::isKeyDown(KeyCode::X)) {
		Window::close();
		return;
	}*/

	Debug::update(renderer.camera, 1.0f / 60.0f);

	if (client.IsConnected()) {
		processMessages();
	}

	switch (state)
	{
	case MainLoop::State::MENU:
		menu();
		break;

	case MainLoop::State::GAME:
		if (client.IsConnected()) {
			game.update();
		} else {
			state = State::MENU;
		}
		break;
	default:
		break;
	}

	renderer.update();
	// Should you call sent packets if it isn't connected?
	client.SendPackets(); // Could send packets eailer, maybe before render.
}

void MainLoop::menu() {
	using namespace ImGui;

	const auto& io = GetIO();
	SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	SetNextWindowSize(ImVec2(0.0f, 0.0f));

	Begin("menu", nullptr, ImGuiWindowFlags_NoCollapse);

	static char input[20] = "127.0.0.1";

	{
		BeginDisabled(client.IsConnecting());
		InputText("address", input, sizeof(input));
		EndDisabled();
	}

	if (!client.IsConnecting()) {
		if (Button("connect")) {
			connect(yojimbo::Address(input, SERVER_PORT));
		}
	} else {
		if (Button("stop connecting")) {
			client.Disconnect();
		}
	}

	if (client.ConnectionFailed()) {
		PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		Text("failed to connect");
		PopStyleColor();
	}

	ImGui::End();
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
	if (client.IsConnected() && state == State::GAME) {
		game.processMessage(message);
	}

	switch (message->GetType()) {
		case GameMessageType::JOIN: {
			if (state == State::GAME) {
				CHECK_NOT_REACHED();
				break;
			}
			const auto msg = static_cast<JoinMessage*>(message);
			GameClient newInstance(*msg, std::move(game));
			// C++ doesn't allow assigning to objects with references or const fields.
			// https://stackoverflow.com/questions/7906127/assignment-operator-with-reference-members
			game.~GameClient();
			new (&game) GameClient(std::move(newInstance));
			state = State::GAME;
			break;
		}
	}
}

void MainLoop::connect(const yojimbo::Address& address) {
	if (!address.IsValid()) {
		return;
	}
	u64 clientId;
	yojimbo::random_bytes((uint8_t*)&clientId, 8);
	const yojimbo::Address addresses[] = {
		address, yojimbo::Address("127.0.0.1")
	};
	client.InsecureConnect(DEFAULT_PRIVATE_KEY, clientId, addresses, std::size(addresses));

	#ifdef DEBUG_NETWORK_SIMULATOR
	put("using network simulator");
	client.SetLatency(DEBUG_NETOWRK_SIMULATOR_LATENCY);
	client.SetJitter(DEBUG_NETOWRK_SIMULATOR_JITTER);
	client.SetPacketLoss(DEBUG_NETOWRK_SIMULATOR_PACKET_LOSS_PERCENT);
	#endif
}

void MainLoop::onDisconnected() {
}

