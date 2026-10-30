#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

enum class PlaybackWindowMode {
    Windowed,
    Fullscreen
};

struct WindowScreenPolicySnapshot {
    PlaybackWindowMode mode{PlaybackWindowMode::Windowed};
    std::int32_t screen_index{-1};
    bool screen_index_valid{false};
    bool valid{false};
};

struct ScreenMoveRequest {
    std::int32_t screen_index{-1};
};

}  // namespace kivo::video::playback_graph
