#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

PlaybackSessionRuntime::PlaybackSessionRuntime(const PlaybackGraphPolicy& policy)
    : policy_(policy), command_registry_(128, 1) {}

CommandToken PlaybackSessionRuntime::open(const OpenRequest& request) noexcept {
    if (request.source_id == 0) {
        return reject(PlaybackGraphError::SourceOpenFailed);
    }

    const auto token = accept_command(GraphCommandKind::Open, true);
    if (!token.accepted()) {
        return token;
    }

    if (state_machine_.state() == PlaybackGraphState::NotCreated) {
        state_machine_.transition_to(PlaybackGraphState::Created);
    }
    state_machine_.transition_to(PlaybackGraphState::Building);
    state_machine_.transition_to(PlaybackGraphState::Ready);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    return token;
}

CommandToken PlaybackSessionRuntime::start() noexcept {
    const auto token = accept_command(GraphCommandKind::Start, true);
    if (!token.accepted()) {
        return token;
    }
    state_machine_.transition_to(PlaybackGraphState::Starting);
    return token;
}

CommandToken PlaybackSessionRuntime::pause() noexcept {
    const auto token = accept_command(GraphCommandKind::Pause, true);
    if (!token.accepted()) {
        return token;
    }
    if (state_machine_.state() == PlaybackGraphState::Starting) {
        pending_transport_intent_.request(GraphCommandKind::Pause);
        return token;
    }
    state_machine_.transition_to(PlaybackGraphState::Pausing);
    state_machine_.transition_to(PlaybackGraphState::Paused);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    return token;
}

CommandToken PlaybackSessionRuntime::resume() noexcept {
    const auto token = accept_command(GraphCommandKind::Resume, true);
    if (!token.accepted()) {
        return token;
    }
    state_machine_.transition_to(PlaybackGraphState::Resuming);
    return token;
}

CommandToken PlaybackSessionRuntime::seek(const SeekRequest& request) noexcept {
    if (request.target_timeline_ms < 0) {
        return reject(PlaybackGraphError::InvalidSeekTarget);
    }

    const auto token = accept_command(GraphCommandKind::Seek, true);
    if (!token.accepted()) {
        return token;
    }
    state_machine_.transition_to(PlaybackGraphState::Seeking);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    return token;
}

CommandToken PlaybackSessionRuntime::switch_audio_track(
    const AudioTrackSwitchRequest& request) noexcept {
    if (request.track_id == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SwitchAudioTrack, true);
    if (token.accepted()) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    return token;
}

CommandToken PlaybackSessionRuntime::switch_video_track(
    const VideoTrackSwitchRequest& request) noexcept {
    if (request.track_id == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SwitchVideoTrack, true);
    if (token.accepted()) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    return token;
}

CommandToken PlaybackSessionRuntime::switch_av_track_set(
    const AvTrackSetSwitchRequest& request) noexcept {
    if (request.audio_track_id == 0 && request.video_track_id == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SwitchAvTrackSet, true);
    if (token.accepted()) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    return token;
}

CommandToken PlaybackSessionRuntime::close() noexcept {
    pending_transport_intent_.clear_for_close();
    const auto token = accept_command(GraphCommandKind::Close, false);
    if (!token.accepted()) {
        return token;
    }
    state_machine_.transition_to(PlaybackGraphState::Closing);
    state_machine_.transition_to(PlaybackGraphState::Closed);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    return token;
}

PlaybackSessionSnapshot PlaybackSessionRuntime::snapshot() const noexcept {
    PlaybackSessionSnapshot snapshot{};
    snapshot.session_id = session_id_;
    snapshot.graph_id = graph_id_;
    snapshot.state = state_machine_.state();
    snapshot.playback_revision = p4_adapter_.current_playback_revision();
    snapshot.graph_snapshot_revision = state_machine_.snapshot_revision();
    snapshot.retained_command_count =
        static_cast<std::uint32_t>(command_registry_.active_record_count() +
                                   command_registry_.tombstone_count());
    snapshot.late_event_discard_count = command_registry_.late_event_discard_count();
    snapshot.closed = state_machine_.state() == PlaybackGraphState::Closed;
    return snapshot;
}

CommandLifecycleSnapshot PlaybackSessionRuntime::query_command(PlaybackCommandId id) const noexcept {
    return command_registry_.query(id);
}

SubscriptionToken PlaybackSessionRuntime::subscribe_events(GraphObserverHandle observer) noexcept {
    if (observer.callback == nullptr) {
        return SubscriptionToken{};
    }
    const auto token = next_subscription_token_;
    next_subscription_token_.value += 1;
    return token;
}

void PlaybackSessionRuntime::unsubscribe(SubscriptionToken token) noexcept {
    if (token.value != 0 && token.value < next_subscription_token_.value) {
        return;
    }
}

CommandToken PlaybackSessionRuntime::inject_system_event(const SystemEvent& event) noexcept {
    if (event.kind == SystemEventKind::None) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    return accept_command(GraphCommandKind::InjectSystemEvent, false);
}

CommandToken PlaybackSessionRuntime::accept_command(GraphCommandKind command,
                                                    bool requires_sidecar) noexcept {
    const auto conflict = resolve_command_conflict(command,
                                                   state_machine_.state(),
                                                   pending_transport_intent_.has_pending());
    if (conflict.decision == CommandConflictDecision::RejectClosedGraph ||
        conflict.decision == CommandConflictDecision::RejectInvalidState) {
        return reject(conflict.error);
    }

    CommandCreateRequest request{};
    request.kind = command;
    request.correlation_id = GraphCorrelationId{state_machine_.snapshot_revision().value + 1};
    request.accepted_graph_revision = state_machine_.snapshot_revision();
    request.requires_sidecar = requires_sidecar;

    const auto created = command_registry_.create(request, p4_adapter_);
    if (!created.accepted()) {
        return reject(created.error);
    }

    if (conflict.creates_pending_transport_intent) {
        pending_transport_intent_.request(command);
    }
    return created.token;
}

CommandToken PlaybackSessionRuntime::reject(PlaybackGraphError error) const noexcept {
    CommandToken token{};
    token.immediate_error = error;
    return token;
}

void PlaybackSessionRuntime::complete_if_accepted(CommandToken token,
                                                  CommandTerminalStatus terminal_status,
                                                  PlaybackGraphError error) noexcept {
    if (token.accepted()) {
        (void)command_registry_.complete(token.command_id, terminal_status, error);
    }
}

}  // namespace kivo::video::playback_graph::runtime
