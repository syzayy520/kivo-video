#include "video/playback_graph/local_media/local_media_source_registry.hpp"
#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {
namespace {

[[nodiscard]] std::int64_t fake_duration_ms(std::uint64_t source_id) noexcept {
    return static_cast<std::int64_t>(source_id) * 60000;
}

}  // namespace

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

    const auto local_path =
        local_media::LocalMediaSourceRegistry::instance().resolve(request.source_id);
    if (local_path.has_value()) {
        if (!try_open_local_media(request)) {
            return reject(PlaybackGraphError::SourceOpenFailed);
        }
        position_ms_ = 0;
    } else {
        last_source_id_ = request.source_id;
        duration_ms_ = fake_duration_ms(request.source_id);
        position_ms_ = 0;
        initialize_track_inventory(request.source_id);
    }

    state_machine_.transition_to(PlaybackGraphState::Ready);
    last_seek_target_ms_ = 0;
    seek_in_progress_ = false;
    policy_state_valid_ = true;
    settings_policy_.valid = true;
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
    if (local_media_pipeline_.is_active() && !try_start_local_media()) {
        return reject(PlaybackGraphError::InvalidState);
    }
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

CommandToken PlaybackSessionRuntime::stop() noexcept {
    const auto token = accept_command(GraphCommandKind::Stop, true);
    if (!token.accepted()) {
        return token;
    }
    pending_transport_intent_.clear_for_close();
    const auto current_state = state_machine_.state();
    if (current_state == PlaybackGraphState::Seeking) {
        state_machine_.transition_to(PlaybackGraphState::Ready);
    } else {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
        state_machine_.transition_to(PlaybackGraphState::Ready);
    }
    seek_in_progress_ = false;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
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
    policy_state_valid_ = false;
    settings_policy_.valid = false;
    subtitle_track_id_ = 0;
    subtitle_enabled_ = false;
    subtitle_frame_bridge_.reset();
    local_media_pipeline_.close();
    clear_track_inventory();
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

}  // namespace kivo::video::playback_graph::runtime