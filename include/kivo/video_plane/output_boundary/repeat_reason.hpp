// repeat_reason.hpp — GATE-023: P5A repeat reason
#pragma once
#include <string>

namespace kivo::video_plane::output_boundary {

enum class RepeatReason {
    VrrCadence,                             // repeat for VRR cadence
    FramePacing,                            // repeat for frame pacing
    HoldLastFrame,                          // repeat to hold last frame
    Recovery,                               // repeat during recovery
    Geometry                                // repeat due to geometry change
};

struct RepeatFrameDecision {
    RepeatReason reason{RepeatReason::FramePacing};
    bool should_repeat{false};              // frame should be repeated
    std::string description;                // human-readable description
    bool operator==(RepeatFrameDecision const&) const = default;
};

}  // namespace kivo::video_plane::output_boundary
