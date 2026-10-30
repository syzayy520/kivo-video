#include "video/playback_graph/session/playback_session_runtime.hpp"

#include "video/playback_graph/command/command_conflict_matrix.hpp"
#include "video/playback_graph/local_media/local_media_source_registry.hpp"

namespace kivo::video::playback_graph::runtime {
namespace {

[[nodiscard]] bool contains_track_id(const std::uint64_t* ids,
                                     const std::size_t count,
                                     const std::uint64_t track_id) noexcept {
    for (std::size_t index = 0; index < count; ++index) {
        if (ids[index] == track_id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::int32_t stream_index_from_track_id(const std::uint64_t track_id) noexcept {
    return track_id > 0 ? static_cast<std::int32_t>(track_id - 1) : -1;
}

[[nodiscard]] std::string codec_for_stream(
    const std::int32_t stream_index,
    const std::int32_t* stream_indices,
    const std::string* stream_codecs,
    const std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        if (stream_indices[index] == stream_index) {
            return stream_codecs[index];
        }
    }
    return {};
}

[[nodiscard]] PlaybackGraphState resume_state_after_switch(
    const PlaybackGraphState previous_state) noexcept {
    if (previous_state == PlaybackGraphState::Paused) {
        return PlaybackGraphState::Paused;
    }
    if (previous_state == PlaybackGraphState::Ready) {
        return PlaybackGraphState::Ready;
    }
    return PlaybackGraphState::Playing;
}

[[nodiscard]] bool is_track_switch_state_rejected(
    const CommandConflictResult conflict) noexcept {
    return conflict.decision == CommandConflictDecision::RejectClosedGraph ||
           conflict.decision == CommandConflictDecision::RejectInvalidState;
}

}  // namespace

bool PlaybackSessionRuntime::restart_local_media_with_tracks(
    const std::uint64_t video_track_id,
    const std::uint64_t audio_track_id,
    const std::int64_t restart_position_ms,
    const bool paused_after_restart) noexcept {
    const auto path = local_media::LocalMediaSourceRegistry::instance().resolve(last_source_id_);
    if (!path.has_value() || !local_media_pipeline_.is_active()) {
        return false;
    }

    const auto& probe_status = local_media_pipeline_.status();
    const std::int32_t video_stream_index = stream_index_from_track_id(video_track_id);
    const std::int32_t audio_stream_index = audio_track_id == 0
                                                ? -1
                                                : stream_index_from_track_id(audio_track_id);
    const std::string video_codec =
        codec_for_stream(video_stream_index,
                         probe_status.video_stream_indices,
                         probe_status.video_stream_codecs,
                         probe_status.video_stream_count);
    const std::string audio_codec =
        audio_stream_index >= 0
            ? codec_for_stream(audio_stream_index,
                               probe_status.audio_stream_indices,
                               probe_status.audio_stream_codecs,
                               probe_status.audio_stream_count)
            : std::string{};
    const std::int32_t subtitle_stream_index =
        subtitle_enabled_ && subtitle_track_id_ != 0
            ? stream_index_from_track_id(subtitle_track_id_)
            : -1;
    const std::string subtitle_codec =
        subtitle_stream_index >= 0
            ? codec_for_stream(subtitle_stream_index,
                               probe_status.subtitle_stream_indices,
                               probe_status.subtitle_stream_codecs,
                               probe_status.subtitle_stream_count)
            : std::string{};
    if (video_stream_index < 0 || video_codec.empty()) {
        return false;
    }

    local_media_continuous_.close();
    if (!local_media_continuous_.start(*path,
                                       video_stream_index,
                                       video_codec,
                                       audio_stream_index,
                                       audio_codec,
                                       subtitle_stream_index,
                                       subtitle_codec,
                                       subtitle_enabled_ ? subtitle_track_id_ : 0,
                                       restart_position_ms)) {
        return false;
    }
    if (paused_after_restart) {
        local_media_continuous_.pause();
    }

    const auto& status = local_media_continuous_.stats();
    position_ms_ = status.position_ms;
    video_surface_.ready = status.frame_rendered;
    if (status.frame_rendered) {
        video_surface_.render_state = VideoRenderAttachmentState::SurfaceAttached;
    }
    return true;
}

CommandToken PlaybackSessionRuntime::switch_audio_track(
    const AudioTrackSwitchRequest& request) noexcept {
    if (request.track_id == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto conflict =
        resolve_command_conflict(GraphCommandKind::SwitchAudioTrack,
                                 state_machine_.state(),
                                 pending_transport_intent_.has_pending());
    if (is_track_switch_state_rejected(conflict)) {
        return reject(conflict.error);
    }
    if (!contains_track_id(inventory_audio_ids_, inventory_audio_count_, request.track_id)) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto previous_state = state_machine_.state();
    const auto token = accept_command(GraphCommandKind::SwitchAudioTrack, true);
    if (!token.accepted()) {
        return token;
    }

    const std::int64_t restart_position_ms = current_position_ms();
    if (previous_state != PlaybackGraphState::Ready) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    if (local_media_continuous_.is_active() &&
        !restart_local_media_with_tracks(video_track_id_,
                                         request.track_id,
                                         restart_position_ms,
                                         previous_state == PlaybackGraphState::Paused)) {
        complete_if_accepted(token, CommandTerminalStatus::Failed, PlaybackGraphError::SourceOpenFailed);
        publish_current_snapshot();
        return token;
    }

    audio_track_id_ = request.track_id;
    state_machine_.transition_to(resume_state_after_switch(previous_state));
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::switch_video_track(
    const VideoTrackSwitchRequest& request) noexcept {
    if (request.track_id == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto conflict =
        resolve_command_conflict(GraphCommandKind::SwitchVideoTrack,
                                 state_machine_.state(),
                                 pending_transport_intent_.has_pending());
    if (is_track_switch_state_rejected(conflict)) {
        return reject(conflict.error);
    }
    if (!contains_track_id(inventory_video_ids_, inventory_video_count_, request.track_id)) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto previous_state = state_machine_.state();
    const auto token = accept_command(GraphCommandKind::SwitchVideoTrack, true);
    if (!token.accepted()) {
        return token;
    }

    const std::int64_t restart_position_ms = current_position_ms();
    if (previous_state != PlaybackGraphState::Ready) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    if (local_media_continuous_.is_active() &&
        !restart_local_media_with_tracks(request.track_id,
                                         audio_track_id_,
                                         restart_position_ms,
                                         previous_state == PlaybackGraphState::Paused)) {
        complete_if_accepted(token, CommandTerminalStatus::Failed, PlaybackGraphError::SourceOpenFailed);
        publish_current_snapshot();
        return token;
    }

    video_track_id_ = request.track_id;
    state_machine_.transition_to(resume_state_after_switch(previous_state));
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::switch_av_track_set(
    const AvTrackSetSwitchRequest& request) noexcept {
    if (request.audio_track_id == 0 && request.video_track_id == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const std::uint64_t target_audio =
        request.audio_track_id == 0 ? audio_track_id_ : request.audio_track_id;
    const std::uint64_t target_video =
        request.video_track_id == 0 ? video_track_id_ : request.video_track_id;
    const auto conflict =
        resolve_command_conflict(GraphCommandKind::SwitchAvTrackSet,
                                 state_machine_.state(),
                                 pending_transport_intent_.has_pending());
    if (is_track_switch_state_rejected(conflict)) {
        return reject(conflict.error);
    }
    if (!contains_track_id(inventory_audio_ids_, inventory_audio_count_, target_audio) ||
        !contains_track_id(inventory_video_ids_, inventory_video_count_, target_video)) {
        return reject(PlaybackGraphError::InvalidCommand);
    }

    const auto previous_state = state_machine_.state();
    const auto token = accept_command(GraphCommandKind::SwitchAvTrackSet, true);
    if (!token.accepted()) {
        return token;
    }

    const std::int64_t restart_position_ms = current_position_ms();
    if (previous_state != PlaybackGraphState::Ready) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    if (local_media_continuous_.is_active() &&
        !restart_local_media_with_tracks(target_video,
                                         target_audio,
                                         restart_position_ms,
                                         previous_state == PlaybackGraphState::Paused)) {
        complete_if_accepted(token, CommandTerminalStatus::Failed, PlaybackGraphError::SourceOpenFailed);
        publish_current_snapshot();
        return token;
    }

    audio_track_id_ = target_audio;
    video_track_id_ = target_video;
    state_machine_.transition_to(resume_state_after_switch(previous_state));
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

}  // namespace kivo::video::playback_graph::runtime
