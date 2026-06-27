// waitable_swapchain_state.hpp — GATE-018: P5A waitable swapchain state
#pragma once
#include <cstdint>

namespace kivo::video_plane::vrr {

struct WaitableSwapchainState {
    bool is_waitable{false};                // swapchain is waitable
    uint32_t max_latency{0};                // max frame latency
    uint32_t current_latency{0};            // current frame latency
    bool operator==(WaitableSwapchainState const&) const = default;
};

}  // namespace kivo::video_plane::vrr
