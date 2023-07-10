#include <client/GameClientAdapter.hpp>
#include <client/MainLoop.hpp>

GameClientAdapter::GameClientAdapter(MainLoop& mainLoop) 
    : mainLoop(mainLoop) {}

yojimbo::MessageFactory* GameClientAdapter::CreateMessageFactory(yojimbo::Allocator& allocator) {
    return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
}

void GameClientAdapter::OnServerClientDisconnected(int clientIndex) {
    mainLoop.onDisconnected();
}