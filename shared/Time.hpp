#pragma once

#include <engine/Utils/Types.hpp>

// 2^32 / 1000 / 60 / 60 ~= 1193 hours
// Milliseconds
using ServerClockTime = u32;

#define NETWORK_SERIALIZE_SERVER_CLOCK_TIME(stream, value) serialize_uint32(stream, value)

// The rounding down error in milliseconds ~= 0.666666667.
// Which means it would take 1000 / 0.666666667 ~= 1499.99999925 frames to have the game clock move second faster than real time.
// If a frame takes 16 milliseconds then it takes 1499.99999925  * 0.016 ~= 23.999999988 seconds to speed up by one second. Which is quite quick but it should be ok, because relative time matters and not the speed.
static constexpr u32 FRAME_DT_MILLISECONDS = 16;
static constexpr float FRAME_DT_SECONDS = static_cast<float>(FRAME_DT_MILLISECONDS) / 1000.0f;
static constexpr i32 SERVER_UPDATE_SEND_RATE_DIVISOR = 6;

using FrameTime = i32;

#define NETWORK_SERIALIZE_FRAME_TIME(stream, value) serialize_int(stream, value, 0, INT32_MAX)