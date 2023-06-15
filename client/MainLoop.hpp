#pragma once

#include <client/GameClient.hpp>
#include <client/GameClientAdapter.hpp>

struct MainLoop {
	MainLoop();
	void update();

	void processMessages();
	void processMessage(yojimbo::Message* message);

	void connect(const yojimbo::Address& address);

	GameClientAdapter adapter;
	yojimbo::Client client;

	GameClient game;
	Renderer renderer;
};