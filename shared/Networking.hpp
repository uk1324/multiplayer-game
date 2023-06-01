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

static constexpr int SERVER_UPDATE_SEND_RATE_DIVISOR = 6;

//static constexpr float DEBUG_LATENCY = 1000.0f;
//static constexpr float DEBUG_JITTER = 50.0f;
static constexpr float DEBUG_LATENCY = 0.0f;
static constexpr float DEBUG_JITTER = 0.0f;

namespace GameMessageType {
    enum GameMessageType {
        JOIN,
        CLIENT_INPUT,
        WORLD_UPDATE,
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
        serialize_int(stream, currentFrame, -2, INT_MAX);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct ClientInputMessage : public yojimbo::Message {
    i32 sequenceNumber;
    struct Input {
        bool up = false, down = false, left = false, right = false, shoot = false;
        float rotation = 0.0f;
    };
    static constexpr int INPUTS_COUNT = 4;
    Input inputs[INPUTS_COUNT];

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, sequenceNumber, 0, INT_MAX);
        for (int i = 0; i < std::size(inputs); i++) {
            serialize_bool(stream, inputs[i].left);
            serialize_bool(stream, inputs[i].up);
            serialize_bool(stream, inputs[i].right);
            serialize_bool(stream, inputs[i].down);
            serialize_bool(stream, inputs[i].shoot);
            serialize_float(stream, inputs[i].rotation);
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct WorldUpdateMessage : public yojimbo::BlockMessage {
    i32 lastReceivedClientSequenceNumber;
    i32 sequenceNumber;
    i32 playersCount;
    i32 bulletsCount;

    struct Player {
        PlayerIndex index;
        Vec2 position;
    };

    struct Bullet {
        i32 index;
        Vec2 position;
        Vec2 velocity;
    };

    void set(i32 lastReceivedClientSequenceNumber, i32 sequenceNumber, i32 playersCount, i32 bulletsCount) {
        this->lastReceivedClientSequenceNumber = lastReceivedClientSequenceNumber;
        this->sequenceNumber = sequenceNumber;
        this->playersCount = playersCount;
        this->bulletsCount = bulletsCount;
    }

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, lastReceivedClientSequenceNumber, 0, INT_MAX);
        serialize_int(stream, sequenceNumber, 0, INT_MAX);
        serialize_int(stream, playersCount, 0, INT_MAX);
        serialize_int(stream, bulletsCount, 0, INT_MAX);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, static_cast<int>(GameMessageType::COUNT));
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::JOIN, JoinMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::CLIENT_INPUT, ClientInputMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::WORLD_UPDATE, WorldUpdateMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();
