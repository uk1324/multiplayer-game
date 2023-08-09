#pragma once

#include <yojimbo/yojimbo.h>
#include <engine/Math/Vec2.hpp>
#include <Types.hpp>
#include <shared/MessagesData.hpp>
#include <shared/GameplayStateData.hpp>
#include <limits.h>

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

namespace GameMessageType {
    enum GameMessageType {
        JOIN,
        CLIENT_INPUT,
        WORLD_UPDATE,
        LEADERBOARD_UPDATE,
        SPAWN_REQUEST,
        SPAWN_PLAYER,
        PLAYER_JOINED,
        PLAYER_DISCONNECTED,
        TEST,
        COUNT
    };
};

struct ClientInputMessage : public yojimbo::Message {
    i32 clientSequenceNumber;
    struct Input {
        bool up = false, down = false, left = false, right = false, shoot = false, shift = false;
        float rotation = 0.0f;
        i32 selectedPattern = 0;
        bool operator==(const Input&) const = default;
    };
#define SERIALIZE_INPUT(index) \
    serialize_bool(stream, inputs[index].left); \
    serialize_bool(stream, inputs[index].up); \
    serialize_bool(stream, inputs[index].right); \
    serialize_bool(stream, inputs[index].down); \
    serialize_bool(stream, inputs[index].shoot); \
    serialize_bool(stream, inputs[index].shift); \
    serialize_float(stream, inputs[index].rotation); \
    serialize_int(stream, inputs[index].selectedPattern, 0, std::size(patternInfos) - 1);

    static constexpr int INPUTS_COUNT = 15;
    // Most recent input at the last index.
    Input inputs[INPUTS_COUNT];

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, clientSequenceNumber, 0, INT_MAX);

        SERIALIZE_INPUT(0);

        for (int i = 1; i < std::size(inputs); i++) {
            if (Stream::IsReading) {
                bool repeatLastInput;
                serialize_bool(stream, repeatLastInput);
                if (repeatLastInput) {
                    inputs[i] = inputs[i - 1];
                } else {
                    SERIALIZE_INPUT(i);
                }
            } else {
                bool lastInputRepeated = inputs[i] == inputs[i - 1];
                serialize_bool(stream, lastInputRepeated);
                if (!lastInputRepeated) {
                    SERIALIZE_INPUT(i);
                }
            }
        }
        return true;
    }

#undef SERIALIZE_INPUT

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestMessage : public yojimbo::Message {
    template <typename Stream>
    bool Serialize(Stream& stream) {
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, static_cast<int>(GameMessageType::COUNT));
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::JOIN, JoinMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::CLIENT_INPUT, ClientInputMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::WORLD_UPDATE, WorldUpdateMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::LEADERBOARD_UPDATE, LeaderboardUpdateMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::SPAWN_PLAYER, SpawnPlayerMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::SPAWN_REQUEST, SpawnRequestMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::PLAYER_JOINED, PlayerJoinedMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::PLAYER_DISCONNECTED, PlayerDisconnectedMessage);
//YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::TEST, TestMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();
