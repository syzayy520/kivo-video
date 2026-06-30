#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

enum class SubtitleFrameUnavailableReason : std::uint8_t {
    None = 0,
    SubtitleDisabled,
    NoActiveCue,
    NoRuntime,
    TimelineNotStarted,
};

}  // namespace kivo::video::playback_graph