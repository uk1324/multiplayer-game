#pragma once

#include <engine/Utils/Types.hpp>

using FrameTime = i32;

static constexpr FrameTime FPS = 60;
static constexpr float FRAME_DT_SECONDS = 1.0f / static_cast<float>(FPS);
static constexpr FrameTime SERVER_UPDATE_SEND_RATE_DIVISOR = 6;


#define NETWORK_SERIALIZE_FRAME_TIME(stream, value) serialize_int(stream, value, 0, INT32_MAX)