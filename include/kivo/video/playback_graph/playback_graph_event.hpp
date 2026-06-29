#pragma once

#include "kivo/video/playback_graph/command_lifecycle_snapshot.hpp"
#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"
#include "kivo/video/playback_graph/playback_graph_state.hpp"

namespace kivo::video::playback_graph {

enum class PlaybackGraphEventKind {
    None,
    CommandAccepted,
    CommandCompleted,
    CommandFailed,
    CommandCancelled,
    CommandSuperseded,
    CommandExpired,
    CommandReplayAborted,
    CommandClosedGraphRejected,
    GraphStateChanged,
    GraphFailed,
    GraphClosedFromFailed,
    CriticalEventOverflow,
    MicroAdjustCapabilityChanged,
    ObserverCallbackFailed
};

enum class SystemEventKind {
    None,
    Suspend,
    Resume,
    DeviceChanged,
    SourceHealthChanged,
    P6CapabilityChanged
};

struct SystemEvent {
    SystemEventKind kind{SystemEventKind::None};
    std::uint64_t value{0};
};

struct PlaybackGraphEvent {
    PlaybackGraphEventKind kind{PlaybackGraphEventKind::None};
    PlaybackCommandId command_id{};
    GraphCorrelationId correlation_id{};
    PlaybackGraphState state{PlaybackGraphState::NotCreated};
    PlaybackGraphError error{PlaybackGraphError::None};
    GraphSnapshotRevision snapshot_revision{};
};

}  // namespace kivo::video::playback_graph
