#pragma once

#include <optional>

#include "kivo/video/playback_graph/generation_stamp.hpp"
#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"
#include "kivo/video/playback_graph/playback_graph_state.hpp"

namespace kivo::video::playback_graph::runtime {

struct P4GraphTransitionInput {
    PlaybackCommandId command_id{};
    PlaybackRevision expected_playback_revision{};
    PlaybackRevision observed_playback_revision{};
    GraphSnapshotRevision expected_graph_snapshot_revision{};
    GraphSnapshotRevision observed_graph_snapshot_revision{};
    GenerationStamp expected_generation{};
    GenerationStamp observed_generation{};
    PlaybackGraphState from{PlaybackGraphState::NotCreated};
    PlaybackGraphState to{PlaybackGraphState::NotCreated};
};

struct P4GraphTransitionResult {
    bool accepted{false};
    PlaybackGraphError error{PlaybackGraphError::None};
    PlaybackRevision committed_playback_revision{};
};

class P4StateSyncAdapter {
public:
    void set_command_allocation_available(bool available) noexcept;

    [[nodiscard]] std::optional<PlaybackCommandId> allocate_command_id() noexcept;
    [[nodiscard]] P4GraphTransitionResult validate_and_commit(
        const P4GraphTransitionInput& input) noexcept;
    [[nodiscard]] PlaybackRevision current_playback_revision() const noexcept;

private:
    bool command_allocation_available_{true};
    std::uint64_t next_command_id_{1};
    PlaybackRevision playback_revision_{};
};

}  // namespace kivo::video::playback_graph::runtime
