#pragma once

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"
#include "kivo/video/playback_graph/playback_graph_state.hpp"

namespace kivo::video::playback_graph::runtime {

struct StateTransitionResult {
    bool accepted{false};
    PlaybackGraphState previous{PlaybackGraphState::NotCreated};
    PlaybackGraphState current{PlaybackGraphState::NotCreated};
    PlaybackGraphError error{PlaybackGraphError::None};
    BuildFailureKind build_failure_kind{BuildFailureKind::None};
    GraphSnapshotRevision snapshot_revision{};
};

class PlaybackGraphStateMachine {
public:
    [[nodiscard]] PlaybackGraphState state() const noexcept;
    [[nodiscard]] BuildFailureKind build_failure_kind() const noexcept;
    [[nodiscard]] GraphSnapshotRevision snapshot_revision() const noexcept;

    StateTransitionResult transition_to(PlaybackGraphState next) noexcept;
    StateTransitionResult mark_build_failed(BuildFailureKind kind) noexcept;

private:
    [[nodiscard]] bool can_transition_to(PlaybackGraphState next) const noexcept;
    StateTransitionResult reject(PlaybackGraphState next, PlaybackGraphError error) const noexcept;
    StateTransitionResult accept(PlaybackGraphState next) noexcept;

    PlaybackGraphState state_{PlaybackGraphState::NotCreated};
    BuildFailureKind build_failure_kind_{BuildFailureKind::None};
    GraphSnapshotRevision snapshot_revision_{};
};

}  // namespace kivo::video::playback_graph::runtime
