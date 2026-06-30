#include "video/playback_graph/session/playback_session_runtime.hpp"

#include <algorithm>

namespace kivo::video::playback_graph::runtime {
namespace {

[[nodiscard]] std::int64_t fake_duration_ms(std::uint64_t source_id) noexcept {
    return static_cast<std::int64_t>(source_id) * 60000;
}

}  // namespace

PlaybackSessionRuntime::PlaybackSessionRuntime(const PlaybackGraphPolicy& policy)
    : policy_(policy), command_registry_(128, 1) {}

void PlaybackSessionRuntime::sync_clock_store() noexcept {
    snapshot_store_.set_clock_estimate(position_ms_, duration_ms_ > 0);
}

void PlaybackSessionRuntime::publish_current_snapshot() noexcept {
    sync_clock_store();
    (void)snapshot_store_.publish(snapshot());
}

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
    last_source_id_ = request.source_id;
    duration_ms_ = fake_duration_ms(request.source_id);
    position_ms_ = 0;
    last_seek_target_ms_ = 0;
    seek_in_progress_ = false;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::start() noexcept {
    const auto token = accept_command(GraphCommandKind::Start, true);
    if (!token.accepted()) {
        return token;
    }
    state_machine_.transition_to(PlaybackGraphState::Starting);
    state_machine_.transition_to(PlaybackGraphState::Playing);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
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
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::resume() noexcept {
    const auto token = accept_command(GraphCommandKind::Resume, true);
    if (!token.accepted()) {
        return token;
    }
    state_machine_.transition_to(PlaybackGraphState::Resuming);
    state_machine_.transition_to(PlaybackGraphState::Playing);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
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

    const std::int64_t target_ms =
        request.kind == SeekKind::Relative
            ? position_ms_ + request.target_timeline_ms
            : request.target_timeline_ms;
    if (target_ms < 0 || (duration_ms_ > 0 && target_ms > duration_ms_)) {
        return reject(PlaybackGraphError::InvalidSeekTarget);
    }

    seek_in_progress_ = true;
    last_seek_target_ms_ = target_ms;
    state_machine_.transition_to(PlaybackGraphState::Seeking);
    position_ms_ = target_ms;
    seek_in_progress_ = false;
    state_machine_.transition_to(PlaybackGraphState::Playing);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
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
    video_surface_ = {};
    video_surface_.render_state = VideoRenderAttachmentState::SurfaceReleased;
    duration_ms_ = 0;
    position_ms_ = 0;
    seek_in_progress_ = false;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::attach_video_surface(
    const VideoSurfaceBindingRequest& request) noexcept {
    if (request.native_handle == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    if (state_machine_.state() == PlaybackGraphState::Closed ||
        state_machine_.state() == PlaybackGraphState::NotCreated) {
        return reject(PlaybackGraphError::InvalidState);
    }

    video_surface_.ready = true;
    video_surface_.width = request.viewport_width > 0 ? request.viewport_width : 1920;
    video_surface_.height = request.viewport_height > 0 ? request.viewport_height : 1080;
    video_surface_.aspect_ratio =
        video_surface_.height > 0
            ? static_cast<double>(video_surface_.width) /
                  static_cast<double>(video_surface_.height)
            : 0.0;
    video_surface_.render_state = VideoRenderAttachmentState::SurfaceAttached;
    publish_current_snapshot();

    CommandToken token{};
    token.command_id.value = 1;
    return token;
}

CommandToken PlaybackSessionRuntime::detach_video_surface() noexcept {
    if (video_surface_.render_state != VideoRenderAttachmentState::SurfaceAttached) {
        return reject(PlaybackGraphError::InvalidState);
    }
    video_surface_.ready = false;
    video_surface_.render_state = VideoRenderAttachmentState::SurfaceDetached;
    publish_current_snapshot();

    CommandToken token{};
    token.command_id.value = 1;
    return token;
}

CommandToken PlaybackSessionRuntime::release_video_surface() noexcept {
    video_surface_ = {};
    video_surface_.render_state = VideoRenderAttachmentState::SurfaceReleased;
    publish_current_snapshot();

    CommandToken token{};
    token.command_id.value = 1;
    return token;
}

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

PlaybackTimelineSnapshot PlaybackSessionRuntime::query_timeline() const noexcept {
    PlaybackTimelineSnapshot timeline{};
    if (state_machine_.state() == PlaybackGraphState::Closed ||
        state_machine_.state() == PlaybackGraphState::NotCreated) {
        return timeline;
    }

    timeline.position_ms = position_ms_;
    timeline.duration_ms = duration_ms_;
    timeline.seek_in_progress = seek_in_progress_;
    timeline.last_seek_target_ms = last_seek_target_ms_;
    timeline.valid = duration_ms_ > 0;

    const auto audio = snapshot_store_.query_audio_queue();
    const auto video = snapshot_store_.query_video_queue();
    if (audio.ok() && video.ok()) {
        const auto buffered_end = position_ms_ + static_cast<std::int64_t>(audio.value.queued_duration_ms);
        timeline.buffered_range.start_ms = position_ms_;
        timeline.buffered_range.end_ms =
            duration_ms_ > 0 ? std::min(buffered_end, duration_ms_) : buffered_end;
    }

    return timeline;
}

VideoSurfaceSnapshot PlaybackSessionRuntime::query_video_surface() const noexcept {
    return video_surface_;
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

SnapshotQueryResult<ClockSnapshot> PlaybackSessionRuntime::query_clock() const noexcept {
    auto clock = snapshot_store_.query_clock();
    if (clock.ok()) {
        clock.value.estimate_ms = position_ms_;
        clock.value.valid = duration_ms_ > 0;
    }
    return clock;
}

SnapshotQueryResult<AudioQueueSnapshot> PlaybackSessionRuntime::query_audio_queue() const noexcept {
    return snapshot_store_.query_audio_queue();
}

SnapshotQueryResult<VideoQueueSnapshot> PlaybackSessionRuntime::query_video_queue() const noexcept {
    return snapshot_store_.query_video_queue();
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
