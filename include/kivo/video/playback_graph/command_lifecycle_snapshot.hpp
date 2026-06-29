#pragma once

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"

namespace kivo::video::playback_graph {

enum class CommandLifecycleState {
    NotFound,
    Accepted,
    Enqueued,
    Running,
    Completed,
    Failed,
    Cancelled,
    Superseded,
    Expired,
    Tombstoned
};

struct CommandLifecycleSnapshot {
    PlaybackCommandId command_id{};
    GraphCorrelationId correlation_id{};
    CommandLifecycleState state{CommandLifecycleState::NotFound};
    CommandTerminalStatus terminal_status{CommandTerminalStatus::None};
    PlaybackGraphError error{PlaybackGraphError::None};
    GraphSnapshotRevision accepted_snapshot_revision{};
    GraphSnapshotRevision last_update_snapshot_revision{};
};

}  // namespace kivo::video::playback_graph
