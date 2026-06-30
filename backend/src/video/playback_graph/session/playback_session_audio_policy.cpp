#include "video/playback_graph/session/playback_session_runtime.hpp"

#include <cstring>

namespace kivo::video::playback_graph::runtime {

CommandToken PlaybackSessionRuntime::set_audio_volume(const AudioVolumeRequest& request) noexcept {
    if (request.volume < 0.0 || request.volume > 1.0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SetAudioOutputPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    audio_volume_ = request.volume;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::set_audio_muted(const AudioMuteRequest& request) noexcept {
    const auto token = accept_command(GraphCommandKind::SetAudioOutputPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    audio_muted_ = request.muted;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::select_audio_output_device(
    const AudioDeviceSelectRequest& request) noexcept {
    if (request.device_id[0] == '\0') {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SetAudioOutputPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    std::strncpy(audio_device_id_, request.device_id, sizeof(audio_device_id_) - 1);
    audio_device_id_[sizeof(audio_device_id_) - 1] = '\0';
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::set_audio_delay(const AudioDelayRequest& request) noexcept {
    const auto token = accept_command(GraphCommandKind::SetAudioOutputPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    audio_delay_ms_ = request.delay_ms;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

AudioOutputPolicySnapshot PlaybackSessionRuntime::query_audio_output_policy() const noexcept {
    AudioOutputPolicySnapshot policy{};
    if (!policy_state_valid_) {
        return policy;
    }
    policy.volume = audio_volume_;
    policy.muted = audio_muted_;
    std::strncpy(policy.device_id, audio_device_id_, sizeof(policy.device_id) - 1);
    policy.device_id[sizeof(policy.device_id) - 1] = '\0';
    policy.delay_ms = audio_delay_ms_;
    policy.valid = true;
    return policy;
}

}  // namespace kivo::video::playback_graph::runtime