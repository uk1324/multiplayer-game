#pragma once

#include <client/GameClient.hpp>
#include <client/GameClientAdapter.hpp>

// In a ecs if some entites have a gravity componenet all of them get affected by gravity.
// If you wanted to for example implement flight for only the player you would either need to counteract gravity (would need to know how gravity is applied). Store some boolean to disable gravity. Create new component just for the player to update only when some bool is enabled.
// The last approach would be similar to just storing an array of structs of player and a separate array of struct of entity and updating them seperately. The entites would only store what the need and it would be more explicit what components are this entity can have for example there can be optional componenets that are sometimes active which would require extra work in an ecs (for example storing an active flag with an componenet and only allow adding componenets are creation). The ecs approach is more flexible if you don't know what componenets an entity needs at compile time. It might also be simpler to use in a editor, because you don't need to recompile to test what if you add a componenet to an entity.
// In both cases you can utilize using AoS instead of SoA. If the ecs system is too rigid it might not be possible to for example store a bitset istead of an array of bools.
// Might use codegen to generate the AoS struct. Internally it would be represented as an AoS, but it would also be accessible by converting it into a struct. Which to prevent duplication could be generated from a .data struct specification.

// Systems could be implemented as just functions or classes that get passed a context struct. The main loop would pass all the data needed to the struct. If the editor could detect if a struct filed isn't used than you could remove it. With the informtation of what data the context has you might be able to make a compile time multithreaded task scheduler.
struct MainLoop {
	MainLoop();
	~MainLoop();
	void update();

	void processMessages();
	void processMessage(yojimbo::Message* message);

	void connect(const yojimbo::Address& address);

	void onDisconnected();

	GameClientAdapter adapter;
	yojimbo::Client client;

	GameClient game;
	Renderer renderer;
};