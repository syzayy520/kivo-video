#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::control {

/// ControlEventQueuePolicy defines control event queue limits.
struct ControlEventQueuePolicy
{
    primitives::Count normal_max_depth{};
    primitives::Count critical_reserved_depth{};
    bool coalesce_high_frequency_events{true};
};

/// CriticalEventQueueKind enumerates which queue an event belongs to.
/// CriticalEventQueue: Stop, Shutdown, Error, Recovery, RevisionBarrier,
///   DownstreamFailure, DownstreamAck, TrackSwitchFenceTimeout.
/// NormalEventQueue: BufferUpdated, DiagnosticsUpdated, DemandHint,
///   ReadCompleted, TimelineUpdated.
enum class CriticalEventQueueKind
{
    Critical,
    Normal
};

} // namespace kivo::playback::control_plane::control
