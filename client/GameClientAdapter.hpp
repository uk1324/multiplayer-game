#pragma once

#include <shared/Networking.hpp>

struct MainLoop;

class GameClientAdapter : public yojimbo::Adapter {
public:
    GameClientAdapter(MainLoop& mainLoop);

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override;

    void OnServerClientDisconnected(int clientIndex) override;

    MainLoop& mainLoop;
};