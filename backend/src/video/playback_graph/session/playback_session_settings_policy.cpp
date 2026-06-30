#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

CommandToken PlaybackSessionRuntime::set_aspect_mode(
    const PlaybackAspectModeRequest& request) noexcept {
    const auto token = accept_command(GraphCommandKind::SetPlaybackSettingsPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    settings_policy_.aspect = request.mode;
    settings_policy_.valid = policy_state_valid_;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::set_scale_mode(const PlaybackScaleModeRequest& request) noexcept {
    const auto token = accept_command(GraphCommandKind::SetPlaybackSettingsPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    settings_policy_.scale = request.mode;
    settings_policy_.valid = policy_state_valid_;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::set_tone_mapping_mode(
    const PlaybackToneMappingModeRequest& request) noexcept {
    const auto token = accept_command(GraphCommandKind::SetPlaybackSettingsPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    settings_policy_.tone_mapping = request.mode;
    settings_policy_.valid = policy_state_valid_;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::set_deinterlace_mode(
    const PlaybackDeinterlaceModeRequest& request) noexcept {
    const auto token = accept_command(GraphCommandKind::SetPlaybackSettingsPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    settings_policy_.deinterlace = request.mode;
    settings_policy_.valid = policy_state_valid_;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::set_playback_speed(const PlaybackSpeedRequest& request) noexcept {
    if (request.speed <= 0.0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SetPlaybackSettingsPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    settings_policy_.playback_speed = request.speed;
    settings_policy_.valid = policy_state_valid_;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::set_subtitle_size(const SubtitleSizeRequest& request) noexcept {
    if (request.scale <= 0.0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SetPlaybackSettingsPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    settings_policy_.subtitle_size = request.scale;
    settings_policy_.valid = policy_state_valid_;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

PlaybackSettingsPolicySnapshot PlaybackSessionRuntime::query_playback_settings_policy()
    const noexcept {
    if (!policy_state_valid_) {
        return PlaybackSettingsPolicySnapshot{};
    }
    return settings_policy_;
}

}  // namespace kivo::video::playback_graph::runtime