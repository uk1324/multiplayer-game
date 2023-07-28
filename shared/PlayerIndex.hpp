#pragma once

#include <Types.hpp>
#include <ostream>

// Not using a type alias to prevent implicit conversions from clientIndex.
struct PlayerIndex {
    u32 value;

    bool operator==(const PlayerIndex&) const = default;
};

std::ostream& operator<< (std::ostream& os, const PlayerIndex& playerIndex);

template<>
struct std::hash<PlayerIndex> {
    std::size_t operator()(const PlayerIndex& value) const noexcept {
        return value.value;
    }
};

#define NETWORK_SERIALIZE_PLAYER_INDEX(stream, playerIndexValue) serialize_uint32(stream, (playerIndexValue).value)