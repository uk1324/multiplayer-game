#pragma once

#include <shared/Networking.hpp>

class GameClientAdapter : public yojimbo::Adapter {
public:
    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override;
};