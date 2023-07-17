#pragma once

#include <yojimbo/yojimbo.h>
#include <engine/Math/Vec2.hpp>
#include <Types.hpp>
#include <shared/MessagesData.hpp>
#include <shared/GameplayStateData.hpp>
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

namespace GameMessageType {
    enum GameMessageType {
        JOIN,
        CLIENT_INPUT,
        WORLD_UPDATE,
        LEADERBOARD_UPDATE,
        SPAWN_REQUEST,
        SPAWN_PLAYER,
        TEST,
        COUNT
    };
};

struct ClientInputMessage : public yojimbo::Message {
    i32 clientSequenceNumber;
    struct Input {
        bool up = false, down = false, left = false, right = false, shoot = false, shift = false;
        float rotation = 0.0f;
    };
    static constexpr int INPUTS_COUNT = 15;
    // Most recent input at the last index.
    Input inputs[INPUTS_COUNT];

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, clientSequenceNumber, 0, INT_MAX);
        for (int i = 0; i < std::size(inputs); i++) {
            serialize_bool(stream, inputs[i].left);
            serialize_bool(stream, inputs[i].up);
            serialize_bool(stream, inputs[i].right);
            serialize_bool(stream, inputs[i].down);
            serialize_bool(stream, inputs[i].shoot);
            serialize_bool(stream, inputs[i].shift);
            serialize_float(stream, inputs[i].rotation);
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct LeaderboardUpdateMessage : public yojimbo::BlockMessage {
    struct Entry {
        int playerIndex = -1;
        int deaths = -1;
        int kills = -1;

        friend std::ostream& operator<<(std::ostream& os, const Entry& entry);
    };
    int entryCount;

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, entryCount, 1, INT_MAX);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct SpawnMessage : public yojimbo::Message {
    i32 playerIndex;
    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, playerIndex, 0, MAX_CLIENTS);
        return true;
    }

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
YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::TEST, TestMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();
