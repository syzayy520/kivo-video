#include "video/playback_graph/session/playback_session_runtime.hpp"

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

[[nodiscard]] std::int32_t subtitle_stream_index_from_track_id(
    const std::uint64_t track_id) noexcept {
    return track_id > 0 ? static_cast<std::int32_t>(track_id - 1) : -1;
}

[[nodiscard]] std::string subtitle_codec_for_stream(
    const std::int32_t stream_index,
    const kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus&
        status) {
    for (std::size_t index = 0; index < status.subtitle_stream_count; ++index) {
        if (status.subtitle_stream_indices[index] == stream_index) {
            return status.subtitle_stream_codecs[index];
        }
    }
    return {};
}

}  // namespace

CommandToken PlaybackSessionRuntime::switch_subtitle_track(
    const SubtitleTrackSwitchRequest& request) noexcept {
    if (request.track_id == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    if (!contains_track_id(inventory_subtitle_ids_,
                           inventory_subtitle_count_,
                           request.track_id)) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto previous_state = state_machine_.state();
    const auto token = accept_command(GraphCommandKind::SwitchSubtitleTrack, true);
    if (!token.accepted()) {
        return token;
    }
    if (previous_state != PlaybackGraphState::Ready) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    subtitle_track_id_ = request.track_id;
    subtitle_enabled_ = true;
    if (local_media_continuous_.is_active()) {
        const auto& status = local_media_pipeline_.status();
        const std::int32_t stream_index =
            subtitle_stream_index_from_track_id(subtitle_track_id_);
        local_media_continuous_.select_subtitle_track(
            subtitle_track_id_,
            stream_index,
            subtitle_codec_for_stream(stream_index, status));
    } else {
        subtitle_frame_bridge_.attach(subtitle_track_id_, subtitle_fixture_source_allowed_);
    }
    state_machine_.transition_to(resume_state_after_switch(previous_state));
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
    local_media_continuous_.disable_subtitle();
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
