#pragma once

#include <optional>
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::feedback {

/// DownstreamAckKind enumerates downstream acknowledgment kinds.
enum class DownstreamAckKind
{
    SeekFenceAck,
    TrackSwitchFenceAck,
    DiscontinuityAck,
    StreamRevisionAck,
    BufferFlushAck,
    TimelineResyncAck
};

/// StreamType enumerates stream kinds for feedback.
enum class StreamType
{
    Video,
    Audio,
    Subtitle,
    Unknown
};

/// DownstreamDemandHintRequest carries downstream demand hints.
struct DownstreamDemandHintRequest
{
    revision::StreamRevision stream_revision{};
    bool decoder_waiting_for_data{false};
    bool downstream_starved{false};
    bool downstream_waiting_for_discontinuity_data{false};
    bool after_seek_warmup{false};
    std::optional<primitives::Bytes> preferred_next_bytes{};
    std::optional<primitives::MediaTimeMs> preferred_next_time_ms{};
    StreamType stream_type{StreamType::Unknown};
    primitives::MonotonicTimestamp submitted_at{};
};

/// DownstreamDemandHint wraps a demand hint with a feedback id.
struct DownstreamDemandHint
{
    revision::FeedbackId id{};
    DownstreamDemandHintRequest request{};
};

/// DownstreamAck is a critical-lane downstream acknowledgment.
struct DownstreamAck
{
    revision::StreamRevision stream_revision{};
    revision::TimelineRevision timeline_revision{};
    revision::SessionGeneration generation{};
    std::optional<revision::TrackSwitchFenceId> fence_id{};
    std::optional<primitives::U64> track_switch_plan_id{};
    DownstreamAckKind kind{DownstreamAckKind::SeekFenceAck};
    primitives::MonotonicTimestamp submitted_at{};
};

/// PlaybackFailureRef is a reference to a playback failure (defined in recovery).
struct PlaybackFailureRef
{
    primitives::U32 stable_code{0};
    StreamType stream_type{StreamType::Unknown};
};

/// DownstreamFailure is a critical-lane downstream failure report.
struct DownstreamFailure
{
    revision::SessionGeneration generation{};
    revision::TimelineRevision timeline_revision{};
    revision::StreamRevision stream_revision{};
    StreamType stream_type{StreamType::Unknown};
    PlaybackFailureRef failure{};
};

/// DownstreamFeedbackQueuePolicy defines feedback queue limits.
struct DownstreamFeedbackQueuePolicy
{
    primitives::Count normal_max_depth{};
    primitives::Count critical_reserved_depth{};
    bool coalesce_demand_hint{true};
};

} // namespace kivo::playback::control_plane::feedback
