// catchup_policy.hpp — GATE-020: P5A catchup policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::timebase {

enum class CatchupAction {
    None,                                   // no catchup needed
    SkipFrames,                             // skip frames to catch up
    ReduceQuality,                          // reduce decode quality
    IncreasePlaybackRate,                   // temporarily increase playback rate
    FailToUpperLayer                        // cannot catch up, fail
};

struct CatchupPolicy {
    CatchupAction action{CatchupAction::None};
    uint64_t catchup_threshold_ms{1000};    // threshold before catchup triggers
    uint32_t max_skip_count{5};             // max frames to skip at once
    bool operator==(CatchupPolicy const&) const = default;
};

}  // namespace kivo::video_plane::timebase
