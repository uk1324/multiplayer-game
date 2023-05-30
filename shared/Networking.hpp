#pragma once

#include <yojimbo/yojimbo.h>
#include <Shared/Math/vec2.hpp>
#include <Shared/Utils/Types.hpp>
#include <bit>

namespace GameChannel {
    enum GameChannel {
        RELIABLE,
        UNRELIABLE,
        COUNT
    };
}

struct GameConnectionConfig : yojimbo::ClientServerConfig {
    GameConnectionConfig() {
        numChannels = 2;
        channel[GameChannel::RELIABLE].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
        channel[GameChannel::UNRELIABLE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    }
};

extern GameConnectionConfig connectionConfig;
static constexpr int SERVER_PORT = 40001;
static constexpr int CLIENT_PORT = 30001;

static const uint8_t DEFAULT_PRIVATE_KEY[yojimbo::KeyBytes] = { 0 };
static const int MAX_CLIENTS = 64;


namespace GameMessageType {
    enum GameMessageType {
        JOIN,
        CLIENT_INPUT,
        PLAYER_POSITION_UPDATE,
        COUNT
    };
};

using PlayerIndex = int;

class JoinMessage : public yojimbo::Message {
public:
    PlayerIndex clientPlayerIndex = 0;
    int currentFrame = 0;

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, clientPlayerIndex, 0, MAX_CLIENTS);
        serialize_int(stream, currentFrame, 0, INT_MAX);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};


//struct JoinGameMessage : public yojimbo::Message {
//    PlayerId clientIndex;
//
//    template <typename Stream>
//    bool Serialize(Stream& stream) {
//        //serialize_uint64(stream, static_cast<u64>(clientIndex));
//        return true;
//    }
//
//    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
//};

struct ClientInputMessage : public yojimbo::Message {
    int frame;
    bool left = false, up = false, right = false, down = false;

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, frame, 0, INT_MAX);
        serialize_bool(stream, left);
        serialize_bool(stream, up);
        serialize_bool(stream, right);
        serialize_bool(stream, down);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct PlayerPositionUpdateMessage : public yojimbo::BlockMessage {
    int lastReceivedInputFrameValue;

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, lastReceivedInputFrameValue, 0, INT_MAX);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct PlayerPosition {
    PlayerIndex playerIndex;
    Vec2 position;
};

YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, static_cast<int>(GameMessageType::COUNT));
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::JOIN, JoinMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::CLIENT_INPUT, ClientInputMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::PLAYER_POSITION_UPDATE, PlayerPositionUpdateMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();
