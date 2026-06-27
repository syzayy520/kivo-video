#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/timeline/time_mapping_confidence.hpp"

namespace kivo::playback::control_plane::live {

/// LiveWindow captures a live seekable window.
struct LiveWindow
{
    primitives::MediaTimeMs window_start_ms{};
    primitives::MediaTimeMs window_end_ms{};
    bool seekable_within_window{false};
    bool duration_dynamic{false};
};

/// LiveWindowObservation is a live window observation from P3/P5/P6 feedback.
struct LiveWindowObservation
{
    revision::SessionId session_id{};
    revision::P4RuntimePolicyRevision runtime_revision{};
    revision::TimelineRevision based_on_timeline_revision{};
    LiveWindow live_window{};
    timeline::TimeMappingConfidence confidence{timeline::TimeMappingConfidence::None};
    primitives::MonotonicTimestamp observed_at{};
};

} // namespace kivo::playback::control_plane::live
