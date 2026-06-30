#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "video/player_runtime_adapter/commands/adapter_p7_request_mappers.hpp"
#include "video/player_runtime_adapter/p7_bridge/p7_command_result.hpp"

namespace kivo::video::player_runtime_adapter {

AdapterCommandResult PlayerRuntimeAdapter::set_volume(const double value) noexcept {
    playback_graph::AudioVolumeRequest request{};
    request.volume = value;
    return runtime::command_result_from_p7(session_.set_audio_volume(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_muted(const bool muted) noexcept {
    playback_graph::AudioMuteRequest request{};
    request.muted = muted;
    return runtime::command_result_from_p7(session_.set_audio_muted(request));
}

AdapterCommandResult PlayerRuntimeAdapter::select_audio_device(
    const std::string_view device_id) noexcept {
    playback_graph::AudioDeviceSelectRequest request{};
    commands::copy_device_id(request, device_id);
    return runtime::command_result_from_p7(session_.select_audio_output_device(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_subtitle_size(const double scale) noexcept {
    playback_graph::SubtitleSizeRequest request{};
    request.scale = scale;
    return runtime::command_result_from_p7(session_.set_subtitle_size(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_subtitle_delay(const std::int64_t delay_ms) noexcept {
    playback_graph::SubtitleDelayRequest request{};
    request.delay_ms = delay_ms;
    return runtime::command_result_from_p7(session_.set_subtitle_delay(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_audio_delay(const std::int64_t delay_ms) noexcept {
    playback_graph::AudioDelayRequest request{};
    request.delay_ms = delay_ms;
    return runtime::command_result_from_p7(session_.set_audio_delay(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_aspect_mode(const AdapterAspectMode mode) noexcept {
    playback_graph::PlaybackAspectModeRequest request{};
    request.mode = commands::to_p7_aspect(mode);
    return runtime::command_result_from_p7(session_.set_aspect_mode(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_scale_mode(const AdapterScaleMode mode) noexcept {
    playback_graph::PlaybackScaleModeRequest request{};
    request.mode = commands::to_p7_scale(mode);
    return runtime::command_result_from_p7(session_.set_scale_mode(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_tone_mapping_mode(
    const AdapterToneMappingMode mode) noexcept {
    playback_graph::PlaybackToneMappingModeRequest request{};
    request.mode = commands::to_p7_tone(mode);
    return runtime::command_result_from_p7(session_.set_tone_mapping_mode(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_deinterlace_mode(
    const AdapterDeinterlaceMode mode) noexcept {
    playback_graph::PlaybackDeinterlaceModeRequest request{};
    request.mode = commands::to_p7_deinterlace(mode);
    return runtime::command_result_from_p7(session_.set_deinterlace_mode(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_playback_speed(const double speed) noexcept {
    playback_graph::PlaybackSpeedRequest request{};
    request.speed = speed;
    return runtime::command_result_from_p7(session_.set_playback_speed(request));
}

}  // namespace kivo::video::player_runtime_adapter