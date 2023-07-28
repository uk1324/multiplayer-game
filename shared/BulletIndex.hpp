#pragma once

#include <shared/PlayerIndex.hpp>

struct UntypedBulletIndex {
    PlayerIndex ownerPlayerIndex;
    FrameTime spawnFrame;
    u16 spawnIndexInFrame;

    bool operator==(const UntypedBulletIndex&) const = default;
};

template<>
struct std::hash<UntypedBulletIndex> {
    std::size_t operator()(const UntypedBulletIndex& value) const noexcept {
        const auto h1 = std::hash<PlayerIndex>{}(value.ownerPlayerIndex);
        const auto h2 = std::hash<FrameTime>{}(value.spawnFrame);
        const auto h3 = std::hash<u16>{}(value.spawnIndexInFrame);
        return (h1 << h2) * h3 - 1;
    }
};