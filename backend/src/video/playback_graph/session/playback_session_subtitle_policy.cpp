#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

CommandToken PlaybackSessionRuntime::switch_subtitle_track(
    const SubtitleTrackSwitchRequest& request) noexcept {
    if (request.track_id == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SwitchSubtitleTrack, true);
    if (!token.accepted()) {
        return token;
    }
    state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    subtitle_track_id_ = request.track_id;
    subtitle_enabled_ = true;
    subtitle_frame_bridge_.attach(subtitle_track_id_);
    state_machine_.transition_to(PlaybackGraphState::Playing);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::disable_subtitle() noexcept {
    const auto token = accept_command(GraphCommandKind::DisableSubtitle, true);
    if (!token.accepted()) {
        return token;
    }
    subtitle_enabled_ = false;
    subtitle_track_id_ = 0;
    subtitle_frame_bridge_.detach();
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::set_subtitle_delay(const SubtitleDelayRequest& request) noexcept {
    const auto token = accept_command(GraphCommandKind::SetSubtitleDelay, false);
    if (!token.accepted()) {
        return token;
    }
    subtitle_delay_ms_ = request.delay_ms;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

SubtitleSnapshot PlaybackSessionRuntime::query_subtitle() const noexcept {
    SubtitleSnapshot subtitle{};
    if (!policy_state_valid_) {
        return subtitle;
    }
    subtitle.selected_track_id = subtitle_track_id_;
    subtitle.enabled = subtitle_enabled_;
    subtitle.delay_ms = subtitle_delay_ms_;
    subtitle.frame_available = false;
    subtitle.valid = true;
    return subtitle;
}

}  // namespace kivo::video::playback_graph::runtime