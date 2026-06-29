#include "video/playback_graph/coordination/track_switch_coordinator.hpp"

namespace kivo::video::playback_graph::runtime {

TrackSwitchCoordinator::TrackSwitchCoordinator(TrackState state) : state_(state) {}

TrackSwitchOutcome TrackSwitchCoordinator::switch_audio(const AudioTrackSwitchRequest& request,
                                                        bool seek_in_progress) noexcept {
    TrackSwitchOutcome outcome{};
    if (seek_in_progress) {
        outcome.kind = TrackSwitchResultKind::RejectedSeekPriority;
        outcome.error = PlaybackGraphError::InvalidState;
        return outcome;
    }
    if (request.track_id == state_.audio_track_id) {
        outcome.kind = TrackSwitchResultKind::NoOp;
        return outcome;
    }
    state_.audio_track_id = request.track_id;
    outcome.audio_changed = true;
    outcome.micro_adjust_refreshed = state_.p6_micro_adjust_capability_known;
    return outcome;
}

TrackSwitchOutcome TrackSwitchCoordinator::switch_video(const VideoTrackSwitchRequest& request,
                                                        bool seek_in_progress) noexcept {
    TrackSwitchOutcome outcome{};
    if (seek_in_progress) {
        outcome.kind = TrackSwitchResultKind::RejectedSeekPriority;
        outcome.error = PlaybackGraphError::InvalidState;
        return outcome;
    }
    if (request.track_id == state_.video_track_id) {
        outcome.kind = TrackSwitchResultKind::NoOp;
        return outcome;
    }
    state_.video_track_id = request.track_id;
    outcome.video_changed = true;
    return outcome;
}

TrackSwitchOutcome TrackSwitchCoordinator::switch_av(const AvTrackSetSwitchRequest& request,
                                                     bool seek_in_progress,
                                                     bool inject_video_failure,
                                                     bool rollback_success) noexcept {
    TrackSwitchOutcome outcome{};
    if (seek_in_progress) {
        outcome.kind = TrackSwitchResultKind::RejectedSeekPriority;
        outcome.error = PlaybackGraphError::InvalidState;
        return outcome;
    }
    if (request.audio_track_id == state_.audio_track_id &&
        request.video_track_id == state_.video_track_id) {
        outcome.kind = TrackSwitchResultKind::NoOp;
        return outcome;
    }

    const auto previous_audio = state_.audio_track_id;
    state_.audio_track_id = request.audio_track_id;
    outcome.audio_changed = request.audio_track_id != previous_audio;
    outcome.micro_adjust_refreshed = outcome.audio_changed && state_.p6_micro_adjust_capability_known;

    if (inject_video_failure) {
        outcome.rollback_attempted = true;
        if (request.atomicity == TrackSwitchAtomicity::BestEffortPartialAllowed) {
            outcome.kind = TrackSwitchResultKind::Degraded;
            return outcome;
        }
        if (rollback_success) {
            state_.audio_track_id = previous_audio;
            outcome.kind = TrackSwitchResultKind::RolledBack;
            outcome.error = PlaybackGraphError::SourceSeekFailed;
            return outcome;
        }
        outcome.kind = TrackSwitchResultKind::Degraded;
        outcome.error = PlaybackGraphError::RecoveryFailed;
        return outcome;
    }

    state_.video_track_id = request.video_track_id;
    outcome.video_changed = true;
    return outcome;
}

TrackState TrackSwitchCoordinator::state() const noexcept {
    return state_;
}

}  // namespace kivo::video::playback_graph::runtime
