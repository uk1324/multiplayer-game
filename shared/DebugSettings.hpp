#pragma once

#ifndef FINAL_RELEASE
#define DEBUG_NETWORK_SIMULATOR
static constexpr float DEBUG_NETOWRK_SIMULATOR_LATENCY = 150.0f;
static constexpr float DEBUG_NETOWRK_SIMULATOR_JITTER = 50.0f;
static constexpr float DEBUG_NETOWRK_SIMULATOR_PACKET_LOSS_PERCENT = 0.0f;
#endif