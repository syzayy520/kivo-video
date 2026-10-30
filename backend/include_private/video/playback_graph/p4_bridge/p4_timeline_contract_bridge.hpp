#pragma once

#include <cstdint>

#include "kivo/playback/control_plane/timeline/playback_timeline_state.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"
#include "kivo/video/playback_graph/playback_graph_state.hpp"

namespace kivo::video::playback_graph::runtime {

struct P4TimelineContractInput {
    PlaybackSessionId session_id{};
    PlaybackRevision playback_revision{};
    GraphSnapshotRevision graph_snapshot_revision{};
    PlaybackGraphState graph_state{PlaybackGraphState::NotCreated};
    std::int64_t position_ms{0};
    std::int64_t duration_ms{0};
    bool seek_in_progress{false};
    std::int64_t last_seek_target_ms{0};
    bool buffered_range_valid{false};
    std::int64_t buffered_start_ms{0};
    std::int64_t buffered_end_ms{0};
    bool continuous_playback_active{false};
    bool source_seekable{false};
};

struct P4TimelineContractSnapshot {
    bool connected{false};
    bool uses_p4_public_timeline_state{false};
    bool p4_runtime_state_machine_owner{false};
    kivo::playback::control_plane::timeline::PlaybackTimelineState state{};
};

class P4TimelineContractBridge {
public:
    [[nodiscard]] P4TimelineContractSnapshot build(
        const P4TimelineContractInput& input) const noexcept;
};

}  // namespace kivo::video::playback_graph::runtime
