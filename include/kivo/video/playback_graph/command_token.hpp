#pragma once

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"

namespace kivo::video::playback_graph {

struct CommandToken {
    PlaybackCommandId command_id{};
    GraphCorrelationId correlation_id{};
    PlaybackGraphError immediate_error{PlaybackGraphError::None};
    GraphSnapshotRevision accepted_snapshot_revision{};

    [[nodiscard]] bool accepted() const noexcept {
        return immediate_error == PlaybackGraphError::None && command_id.value != 0;
    }
};

}  // namespace kivo::video::playback_graph
