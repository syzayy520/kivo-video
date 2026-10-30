#pragma once

#include <optional>
#include "kivo/playback/control_plane/timeline/time_mapping_confidence.hpp"
#include "kivo/playback/control_plane/timeline/source_timeline_state.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/state/session_state.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::timeline {

/// TimelinePositionConfidence enumerates timeline position confidence levels.
enum class TimelinePositionConfidence
{
    Unknown,
    CommandedEstimate,
    SourceMappedEstimate,
    DownstreamObserved,
    AudioClockStable
};

/// ClockConfidence enumerates clock observation confidence.
enum class ClockConfidence
{
    NoObservation,
    StaleObservation,
    FreshObservation,
    StableClock
};

/// LiveWindow captures a live seekable window.
struct LiveWindow
{
    primitives::MediaTimeMs window_start_ms{};
    primitives::MediaTimeMs window_end_ms{};
    bool seekable_within_window{false};
    bool duration_dynamic{false};
};

/// DriftReport captures clock drift.
struct DriftReport
{
    primitives::Milliseconds drift_ms{};
    primitives::MonotonicTimestamp observed_at{};
};

/// PlaybackTimelineState is the control-plane timeline state.
/// P4 timeline is control-plane timeline.
/// P4 does NOT own decoded frame timestamp.
/// P4 does NOT own audio master clock.
/// P6 may later promote AudioOutput clock to master.
struct PlaybackTimelineState
{
    revision::SessionId session_id{};
    revision::TimelineRevision revision{};
    revision::SessionGeneration generation{};
    state::SessionState state{state::SessionState::Idle};
    primitives::MediaTimeMs commanded_position_ms{};
    primitives::MediaTimeMs observed_position_ms{};
    TimelinePositionConfidence commanded_confidence{TimelinePositionConfidence::Unknown};
    TimelinePositionConfidence observed_confidence{TimelinePositionConfidence::Unknown};
    SourceTimelineState source_timeline{};
    std::optional<primitives::Milliseconds> duration_ms{};
    std::optional<LiveWindow> live_window{};
    std::optional<primitives::MediaTimeMs> pending_seek_target_ms{};
    primitives::Milliseconds playback_rate{1000};
    bool is_seekable{false};
    bool is_buffering{false};
    bool is_eof{false};
    bool is_live_like{false};
    ClockConfidence clock_confidence{ClockConfidence::NoObservation};
    primitives::Milliseconds observation_age{};
    std::optional<DriftReport> drift_report{};
};

} // namespace kivo::playback::control_plane::timeline
