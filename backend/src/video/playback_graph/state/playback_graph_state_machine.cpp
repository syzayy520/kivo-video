#include "video/playback_graph/state/playback_graph_state_machine.hpp"

namespace kivo::video::playback_graph::runtime {

PlaybackGraphState PlaybackGraphStateMachine::state() const noexcept {
    return state_;
}

BuildFailureKind PlaybackGraphStateMachine::build_failure_kind() const noexcept {
    return build_failure_kind_;
}

GraphSnapshotRevision PlaybackGraphStateMachine::snapshot_revision() const noexcept {
    return snapshot_revision_;
}

StateTransitionResult PlaybackGraphStateMachine::transition_to(PlaybackGraphState next) noexcept {
    if (!can_transition_to(next)) {
        return reject(next, next == PlaybackGraphState::Closed ? PlaybackGraphError::ClosedGraph
                                                               : PlaybackGraphError::InvalidState);
    }
    return accept(next);
}

StateTransitionResult PlaybackGraphStateMachine::mark_build_failed(BuildFailureKind kind) noexcept {
    if (state_ != PlaybackGraphState::Building || kind == BuildFailureKind::None) {
        return reject(PlaybackGraphState::BuildFailed, PlaybackGraphError::InvalidState);
    }
    build_failure_kind_ = kind;
    return accept(PlaybackGraphState::BuildFailed);
}

bool PlaybackGraphStateMachine::can_transition_to(PlaybackGraphState next) const noexcept {
    if (state_ == PlaybackGraphState::Closed) {
        return next == PlaybackGraphState::Closed;
    }
    if (next == PlaybackGraphState::Closed) {
        return state_ == PlaybackGraphState::Closing;
    }
    if (next == PlaybackGraphState::Closing) {
        return state_ != PlaybackGraphState::Closed;
    }
    if (state_ == PlaybackGraphState::Closing) {
        return false;
    }

    switch (state_) {
        case PlaybackGraphState::NotCreated:
            return next == PlaybackGraphState::Created || next == PlaybackGraphState::Building;
        case PlaybackGraphState::Created:
            return next == PlaybackGraphState::Building;
        case PlaybackGraphState::Building:
            return next == PlaybackGraphState::Ready || next == PlaybackGraphState::BuildFailed;
        case PlaybackGraphState::BuildFailed:
            return next == PlaybackGraphState::Building;
        case PlaybackGraphState::Ready:
            return next == PlaybackGraphState::Starting || next == PlaybackGraphState::Seeking ||
                   next == PlaybackGraphState::TrackSwitching;
        case PlaybackGraphState::Starting:
            return next == PlaybackGraphState::Playing || next == PlaybackGraphState::Buffering ||
                   next == PlaybackGraphState::Pausing;
        case PlaybackGraphState::Buffering:
            return next == PlaybackGraphState::Playing || next == PlaybackGraphState::Rebuffering ||
                   next == PlaybackGraphState::Stalled || next == PlaybackGraphState::Pausing;
        case PlaybackGraphState::Playing:
            return next == PlaybackGraphState::Pausing || next == PlaybackGraphState::Seeking ||
                   next == PlaybackGraphState::TrackSwitching || next == PlaybackGraphState::Draining ||
                   next == PlaybackGraphState::Rebuffering;
        case PlaybackGraphState::Rebuffering:
            return next == PlaybackGraphState::Playing || next == PlaybackGraphState::Stalled ||
                   next == PlaybackGraphState::Pausing;
        case PlaybackGraphState::Stalled:
            return next == PlaybackGraphState::Recovering || next == PlaybackGraphState::Failed ||
                   next == PlaybackGraphState::Pausing;
        case PlaybackGraphState::Pausing:
            return next == PlaybackGraphState::Paused;
        case PlaybackGraphState::Paused:
            return next == PlaybackGraphState::Resuming || next == PlaybackGraphState::Seeking ||
                   next == PlaybackGraphState::TrackSwitching;
        case PlaybackGraphState::Resuming:
            return next == PlaybackGraphState::Playing || next == PlaybackGraphState::Buffering;
        case PlaybackGraphState::Seeking:
            return next == PlaybackGraphState::Flushing || next == PlaybackGraphState::Ready ||
                   next == PlaybackGraphState::Playing || next == PlaybackGraphState::Paused;
        case PlaybackGraphState::Flushing:
            return next == PlaybackGraphState::Ready || next == PlaybackGraphState::Playing ||
                   next == PlaybackGraphState::Paused;
        case PlaybackGraphState::TrackSwitching:
            return next == PlaybackGraphState::Ready || next == PlaybackGraphState::Playing ||
                   next == PlaybackGraphState::Paused;
        case PlaybackGraphState::Draining:
            return next == PlaybackGraphState::EosReached;
        case PlaybackGraphState::EosReached:
            return next == PlaybackGraphState::Starting || next == PlaybackGraphState::Seeking;
        case PlaybackGraphState::SystemSuspending:
            return next == PlaybackGraphState::SystemSuspended;
        case PlaybackGraphState::SystemSuspended:
            return next == PlaybackGraphState::SystemResuming;
        case PlaybackGraphState::SystemResuming:
            return next == PlaybackGraphState::Ready || next == PlaybackGraphState::Playing ||
                   next == PlaybackGraphState::Paused;
        case PlaybackGraphState::Recovering:
            return next == PlaybackGraphState::Ready || next == PlaybackGraphState::Playing ||
                   next == PlaybackGraphState::Failed;
        case PlaybackGraphState::Failed:
            return next == PlaybackGraphState::Recovering;
        case PlaybackGraphState::Closing:
        case PlaybackGraphState::Closed:
            return false;
    }

    return false;
}

StateTransitionResult PlaybackGraphStateMachine::reject(PlaybackGraphState next,
                                                        PlaybackGraphError error) const noexcept {
    return StateTransitionResult{false, state_, next, error, build_failure_kind_, snapshot_revision_};
}

StateTransitionResult PlaybackGraphStateMachine::accept(PlaybackGraphState next) noexcept {
    const auto previous = state_;
    state_ = next;
    snapshot_revision_.value += 1;
    if (next != PlaybackGraphState::BuildFailed && next != PlaybackGraphState::Building) {
        build_failure_kind_ = BuildFailureKind::None;
    }
    return StateTransitionResult{true,
                                 previous,
                                 state_,
                                 PlaybackGraphError::None,
                                 build_failure_kind_,
                                 snapshot_revision_};
}

}  // namespace kivo::video::playback_graph::runtime
