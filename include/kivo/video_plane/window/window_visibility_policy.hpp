// window_visibility_policy.hpp — GATE-036: P5A window visibility policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::window {

enum class WindowVisibilityState {
    Visible,                                // window is visible
    Occluded,                               // window is occluded
    Minimized,                              // window is minimized
    Hidden                                  // window is hidden
};

struct WindowVisibilityPolicy {
    WindowVisibilityState state{WindowVisibilityState::Visible};
    bool pause_when_occluded{false};        // pause pipeline when occluded
    bool hold_last_frame_when_minimized{true}; // hold last frame when minimized
    std::string reason;                     // reason for state change
    bool operator==(WindowVisibilityPolicy const&) const = default;
};

}  // namespace kivo::video_plane::window
