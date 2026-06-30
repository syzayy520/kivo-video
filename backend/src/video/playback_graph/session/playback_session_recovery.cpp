#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {
namespace {

[[nodiscard]] std::int64_t fake_duration_ms(std::uint64_t source_id) noexcept {
    return static_cast<std::int64_t>(source_id) * 60000;
}

}  // namespace

CommandToken PlaybackSessionRuntime::retry(const RecoveryActionRequest& request) noexcept {
    if (request.kind != RecoveryActionKind::Retry) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    if (state_machine_.state() != PlaybackGraphState::Failed &&
        state_machine_.state() != PlaybackGraphState::BuildFailed) {
        return reject(PlaybackGraphError::InvalidState);
    }

    RecoveryReplayPlan plan{};
    plan.transport = true;
    const auto outcome = recovery_coordinator_.run_fake(plan);
    if (!outcome.accepted) {
        return reject(outcome.error);
    }

    state_machine_.transition_to(PlaybackGraphState::Recovering);
    state_machine_.transition_to(PlaybackGraphState::Ready);
    publish_current_snapshot();

    CommandToken token{};
    token.command_id.value = 1;
    return token;
}

CommandToken PlaybackSessionRuntime::reopen(const RecoveryActionRequest& request) noexcept {
    if (request.kind != RecoveryActionKind::Reopen) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    if (last_source_id_ == 0) {
        return reject(PlaybackGraphError::InvalidState);
    }

    if (state_machine_.state() == PlaybackGraphState::Closed ||
        state_machine_.state() == PlaybackGraphState::NotCreated) {
        OpenRequest open_request{};
        open_request.source_id = last_source_id_;
        return open(open_request);
    }

    state_machine_.transition_to(PlaybackGraphState::Building);
    state_machine_.transition_to(PlaybackGraphState::Ready);
    duration_ms_ = fake_duration_ms(last_source_id_);
    position_ms_ = 0;
    last_seek_target_ms_ = 0;
    seek_in_progress_ = false;
    publish_current_snapshot();

    CommandToken token{};
    token.command_id.value = 1;
    return token;
}

DiagnosticsSummary PlaybackSessionRuntime::query_diagnostics_summary() const noexcept {
    const auto session_snapshot = snapshot();
    DiagnosticsSummary summary{};
    summary.state = session_snapshot.state;
    summary.last_error = session_snapshot.last_error;
    summary.retained_command_count = session_snapshot.retained_command_count;
    summary.dropped_critical_event_count = session_snapshot.dropped_critical_event_count;
    summary.late_event_discard_count = session_snapshot.late_event_discard_count;
    summary.closed = session_snapshot.closed;
    summary.valid = true;
    return summary;
}

}  // namespace kivo::video::playback_graph::runtime