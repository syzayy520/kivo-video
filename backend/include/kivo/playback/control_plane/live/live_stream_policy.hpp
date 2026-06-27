#pragma once

#include "kivo/playback/control_plane/live/live_window.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::live {

/// LivePauseResumeBehavior enumerates live pause/resume behaviors.
enum class LivePauseResumeBehavior
{
    ResumeFromPausedPointIfStillInWindow,
    JumpToLiveEdgeOnResume,
    KeepLatencyTarget,
    PolicyDriven
};

/// LiveWindowExpiredBehavior enumerates expired window behaviors.
enum class LiveWindowExpiredBehavior
{
    JumpToLiveEdge,
    ReportToUpperLayer,
    HardStop
};

/// LiveStreamPolicy defines live stream policy.
/// P4 does NOT parse live manifest.
/// P4 accepts LiveWindowObservation from P3/P5/P6 feedback.
/// If LiveWindowObservation expired, pause resume policy must degrade by
/// LiveWindowExpiredBehavior.
struct LiveStreamPolicy
{
    bool is_live_like{false};
    primitives::Milliseconds target_live_latency_ms{};
    primitives::Milliseconds max_live_latency_ms{};
    bool allow_seek_within_live_window{false};
    bool treat_live_edge_as_eof{false};
    LivePauseResumeBehavior pause_resume_behavior{LivePauseResumeBehavior::PolicyDriven};
    LiveWindowExpiredBehavior window_expired_behavior{LiveWindowExpiredBehavior::ReportToUpperLayer};
};

} // namespace kivo::playback::control_plane::live
