#include "video/playback_graph/p4_bridge/p4_state_sync_adapter.hpp"

namespace kivo::video::playback_graph::runtime {

void P4StateSyncAdapter::set_command_allocation_available(bool available) noexcept {
    command_allocation_available_ = available;
}

std::optional<PlaybackCommandId> P4StateSyncAdapter::allocate_command_id() noexcept {
    if (!command_allocation_available_) {
        return std::nullopt;
    }
    return PlaybackCommandId{next_command_id_++};
}

P4GraphTransitionResult P4StateSyncAdapter::validate_and_commit(
    const P4GraphTransitionInput& input) noexcept {
    if (!(input.expected_playback_revision == input.observed_playback_revision)) {
        return P4GraphTransitionResult{false,
                                       PlaybackGraphError::PlaybackRevisionMismatch,
                                       playback_revision_};
    }
    if (!(input.expected_graph_snapshot_revision == input.observed_graph_snapshot_revision)) {
        return P4GraphTransitionResult{false,
                                       PlaybackGraphError::GraphSnapshotMismatch,
                                       playback_revision_};
    }
    if (!(input.expected_generation == input.observed_generation)) {
        return P4GraphTransitionResult{false,
                                       PlaybackGraphError::GenerationStampMismatch,
                                       playback_revision_};
    }

    playback_revision_.value += 1;
    return P4GraphTransitionResult{true, PlaybackGraphError::None, playback_revision_};
}

PlaybackRevision P4StateSyncAdapter::current_playback_revision() const noexcept {
    return playback_revision_;
}

}  // namespace kivo::video::playback_graph::runtime
