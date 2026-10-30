#pragma once

#include <optional>
#include "kivo/playback/control_plane/events/event_delivery_class.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/snapshot_stamp.hpp"
#include "kivo/playback/control_plane/recovery/playback_failure.hpp"
#include "kivo/playback/control_plane/discontinuity/discontinuity_marker.hpp"
#include "kivo/playback/control_plane/buffer/stream_end_marker.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::events {

/// SessionEventType enumerates session event types.
enum class SessionEventType
{
    // Critical
    Terminated,
    ErrorRaised,
    StopStarted,
    TrackSwitchFenceTimeout,
    // State
    StateChanged,
    SeekStarted,
    SeekCommitted,
    CommandAccepted,
    CommandSuperseded,
    CommandCompleted,
    // Timeline
    TimelineUpdated,
    // Diagnostics
    DiagnosticsUpdated,
    BufferUpdated,
    // Verbose
    DemandHint
};

/// CommandSupersededDetail carries supersession details.
struct CommandSupersededDetail
{
    revision::CommandId superseded_id{};
    revision::CommandId superseded_by_id{};
};

/// SeekEventDetail carries seek event details.
struct SeekEventDetail
{
    primitives::U32 seek_epoch_value{0};
};

/// RecoveryEventDetail carries recovery event details.
struct RecoveryEventDetail
{
    primitives::U32 recovery_epoch_value{0};
};

/// TrackSwitchFenceDetail carries track switch fence details.
struct TrackSwitchFenceDetail
{
    revision::TrackSwitchFenceId fence_id{};
};

/// StreamEndDetail carries stream end details.
struct StreamEndDetail
{
    buffer::EndProjectionKind projection{buffer::EndProjectionKind::TrueEof};
};

/// EventDetail carries optional event-specific details.
struct EventDetail
{
    std::optional<CommandSupersededDetail> command_superseded{};
    std::optional<SeekEventDetail> seek{};
    std::optional<RecoveryEventDetail> recovery{};
    std::optional<TrackSwitchFenceDetail> track_switch_fence{};
    std::optional<StreamEndDetail> stream_end{};
};

/// SessionEvent is a single session event.
struct SessionEvent
{
    SessionEventType type{SessionEventType::StateChanged};
    EventDeliveryClass delivery_class{EventDeliveryClass::State};
    revision::SnapshotStamp stamp{};
    std::optional<revision::CommandId> command_id{};
    std::optional<recovery::PlaybackFailure> failure{};
    std::optional<EventDetail> detail{};
};

} // namespace kivo::playback::control_plane::events
