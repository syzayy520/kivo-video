#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/command_lifecycle_snapshot.hpp"
#include "kivo/video/playback_graph/generation_stamp.hpp"
#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"
#include "kivo/video/playback_graph/playback_graph_state.hpp"

namespace kivo::video::playback_graph {

struct PlaybackSessionSnapshot {
    PlaybackSessionId session_id{};
    PlaybackGraphId graph_id{};
    PlaybackGraphState state{PlaybackGraphState::NotCreated};
    PlaybackGraphError last_error{PlaybackGraphError::None};
    PlaybackRevision playback_revision{};
    GraphSnapshotRevision graph_snapshot_revision{};
    GenerationStamp active_generation{};
    std::uint32_t retained_command_count{0};
    std::uint32_t dropped_critical_event_count{0};
    std::uint32_t late_event_discard_count{0};
    bool closed{false};
};

}  // namespace kivo::video::playback_graph
