#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "kivo/video/playback_graph/seek_request.hpp"
#include "kivo/video/playback_graph/track_switch_request.hpp"
#include "video/player_runtime_adapter/p7_bridge/p7_command_result.hpp"
#include "video/player_runtime_adapter/state/adapter_state_mapper.hpp"

namespace kivo::video::player_runtime_adapter {
namespace {

[[nodiscard]] playback_graph::OpenRequest to_p7_open_request(
    const AdapterOpenRequest& request) noexcept {
    playback_graph::OpenRequest p7_request{};
    p7_request.source_id = request.source_id;
    p7_request.initial_audio_track_id = request.initial_audio_track_id;
    p7_request.initial_video_track_id = request.initial_video_track_id;
    p7_request.audio_selected = request.audio_selected;
    p7_request.video_selected = request.video_selected;
    return p7_request;
}

[[nodiscard]] playback_graph::SeekRequest absolute_seek(std::int64_t position_ms) noexcept {
    playback_graph::SeekRequest request{};
    request.kind = playback_graph::SeekKind::Absolute;
    request.target_timeline_ms = position_ms;
    return request;
}

[[nodiscard]] playback_graph::SeekRequest relative_seek(std::int64_t delta_ms) noexcept {
    playback_graph::SeekRequest request{};
    request.kind = playback_graph::SeekKind::Relative;
    request.target_timeline_ms = delta_ms;
    return request;
}

}  // namespace

PlayerRuntimeAdapter::PlayerRuntimeAdapter() = default;

PlayerRuntimeAdapter::PlayerRuntimeAdapter(const playback_graph::PlaybackGraphPolicy& policy)
    : session_(policy) {}

PlayerRuntimeAdapter::~PlayerRuntimeAdapter() = default;

PlayerRuntimeAdapter::PlayerRuntimeAdapter(PlayerRuntimeAdapter&&) noexcept = default;

PlayerRuntimeAdapter& PlayerRuntimeAdapter::operator=(PlayerRuntimeAdapter&&) noexcept = default;

AdapterCommandResult PlayerRuntimeAdapter::open(const AdapterOpenRequest& request) noexcept {
    return runtime::command_result_from_p7(session_.open(to_p7_open_request(request)));
}

AdapterCommandResult PlayerRuntimeAdapter::open_media_id(std::uint64_t media_id) noexcept {
    AdapterOpenRequest request{};
    request.source_id = media_id;
    return open(request);
}

AdapterCommandResult PlayerRuntimeAdapter::close() noexcept {
    return runtime::command_result_from_p7(session_.close());
}

AdapterCommandResult PlayerRuntimeAdapter::release_surface() noexcept {
    return runtime::command_result_from_p7(session_.release_video_surface());
}

AdapterCommandResult PlayerRuntimeAdapter::attach_video_surface(AdapterSurfaceHandle handle) noexcept {
    playback_graph::VideoSurfaceBindingRequest request{};
    request.native_handle = handle.native_handle;
    return runtime::command_result_from_p7(session_.attach_video_surface(request));
}

AdapterCommandResult PlayerRuntimeAdapter::play() noexcept {
    return runtime::command_result_from_p7(session_.start());
}

AdapterCommandResult PlayerRuntimeAdapter::pause() noexcept {
    return runtime::command_result_from_p7(session_.pause());
}

AdapterCommandResult PlayerRuntimeAdapter::toggle_play_pause() noexcept {
    const auto current = snapshot();
    if (current.is_playing) {
        return pause();
    }
    if (current.is_paused) {
        return runtime::command_result_from_p7(session_.resume());
    }
    return play();
}

AdapterCommandResult PlayerRuntimeAdapter::seek(std::int64_t position_ms) noexcept {
    return runtime::command_result_from_p7(session_.seek(absolute_seek(position_ms)));
}

AdapterCommandResult PlayerRuntimeAdapter::seek_relative(std::int64_t delta_ms) noexcept {
    return runtime::command_result_from_p7(session_.seek(relative_seek(delta_ms)));
}

AdapterCommandResult PlayerRuntimeAdapter::step_forward(std::int64_t step_ms) noexcept {
    return seek_relative(step_ms);
}

AdapterCommandResult PlayerRuntimeAdapter::step_backward(std::int64_t step_ms) noexcept {
    return seek_relative(-step_ms);
}

AdapterCommandResult PlayerRuntimeAdapter::stop() noexcept {
    return runtime::missing_p7_api(AdapterMissingP7Api::StopWithoutClose);
}

AdapterCommandResult PlayerRuntimeAdapter::select_video_track(std::uint64_t track_id) noexcept {
    playback_graph::VideoTrackSwitchRequest request{};
    request.track_id = track_id;
    return runtime::command_result_from_p7(session_.switch_video_track(request));
}

AdapterCommandResult PlayerRuntimeAdapter::select_audio_track(std::uint64_t track_id) noexcept {
    playback_graph::AudioTrackSwitchRequest request{};
    request.track_id = track_id;
    return runtime::command_result_from_p7(session_.switch_audio_track(request));
}

AdapterCommandResult PlayerRuntimeAdapter::select_subtitle_track(std::uint64_t track_id) noexcept {
    (void)track_id;
    return runtime::missing_p7_api(AdapterMissingP7Api::SubtitleTrackSwitch);
}

AdapterCommandResult PlayerRuntimeAdapter::disable_subtitle() noexcept {
    return runtime::missing_p7_api(AdapterMissingP7Api::SubtitleTrackSwitch);
}

AdapterCommandResult PlayerRuntimeAdapter::set_volume(double value) noexcept {
    (void)value;
    return runtime::missing_p7_api(AdapterMissingP7Api::AudioVolume);
}

AdapterCommandResult PlayerRuntimeAdapter::set_muted(bool muted) noexcept {
    (void)muted;
    return runtime::missing_p7_api(AdapterMissingP7Api::AudioVolume);
}

AdapterCommandResult PlayerRuntimeAdapter::select_audio_device(std::string_view device_id) noexcept {
    (void)device_id;
    return runtime::missing_p7_api(AdapterMissingP7Api::AudioDevice);
}

AdapterCommandResult PlayerRuntimeAdapter::enter_fullscreen() noexcept {
    return runtime::missing_p7_api(AdapterMissingP7Api::FullscreenWindowScreen);
}

AdapterCommandResult PlayerRuntimeAdapter::exit_fullscreen() noexcept {
    return runtime::missing_p7_api(AdapterMissingP7Api::FullscreenWindowScreen);
}

AdapterCommandResult PlayerRuntimeAdapter::move_to_screen(std::int32_t screen_index) noexcept {
    (void)screen_index;
    return runtime::missing_p7_api(AdapterMissingP7Api::FullscreenWindowScreen);
}

AdapterCommandResult PlayerRuntimeAdapter::next() noexcept {
    return runtime::missing_p7_api(AdapterMissingP7Api::Playlist);
}

AdapterCommandResult PlayerRuntimeAdapter::previous() noexcept {
    return runtime::missing_p7_api(AdapterMissingP7Api::Playlist);
}

AdapterCommandResult PlayerRuntimeAdapter::play_next_episode() noexcept {
    return runtime::missing_p7_api(AdapterMissingP7Api::Playlist);
}

AdapterCommandResult PlayerRuntimeAdapter::set_auto_play_next(bool enabled) noexcept {
    (void)enabled;
    return runtime::missing_p7_api(AdapterMissingP7Api::Playlist);
}

AdapterCommandResult PlayerRuntimeAdapter::handle_shortcut_name(std::string_view name) noexcept {
    if (name == "togglePlayPause" || name == "playPause") {
        return handle_shortcut(AdapterShortcutAction::TogglePlayPause);
    }
    if (name == "rewind10" || name == "seekBackward") {
        return handle_shortcut(AdapterShortcutAction::SeekBackward);
    }
    if (name == "forward10" || name == "seekForward") {
        return handle_shortcut(AdapterShortcutAction::SeekForward);
    }
    if (name == "volumeUp") {
        return handle_shortcut(AdapterShortcutAction::VolumeUp);
    }
    if (name == "volumeDown") {
        return handle_shortcut(AdapterShortcutAction::VolumeDown);
    }
    if (name == "setMuted" || name == "mute") {
        return handle_shortcut(AdapterShortcutAction::ToggleMute);
    }
    if (name == "enterFullscreen" || name == "fullscreen") {
        return handle_shortcut(AdapterShortcutAction::ToggleFullscreen);
    }
    if (name == "exitFullscreen") {
        return handle_shortcut(AdapterShortcutAction::ExitFullscreen);
    }
    if (name == "subtitle") {
        return handle_shortcut(AdapterShortcutAction::Subtitle);
    }
    if (name == "audioTrack") {
        return handle_shortcut(AdapterShortcutAction::AudioTrack);
    }
    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

AdapterCommandResult PlayerRuntimeAdapter::handle_shortcut(AdapterShortcutAction action) noexcept {
    switch (action) {
        case AdapterShortcutAction::TogglePlayPause:
            return toggle_play_pause();
        case AdapterShortcutAction::SeekBackward:
            return step_backward(10000);
        case AdapterShortcutAction::SeekForward:
            return step_forward(10000);
        case AdapterShortcutAction::VolumeUp:
        case AdapterShortcutAction::VolumeDown:
        case AdapterShortcutAction::ToggleMute:
            return runtime::missing_p7_api(AdapterMissingP7Api::AudioVolume);
        case AdapterShortcutAction::ToggleFullscreen:
        case AdapterShortcutAction::ExitFullscreen:
            return runtime::missing_p7_api(AdapterMissingP7Api::FullscreenWindowScreen);
        case AdapterShortcutAction::Subtitle:
            return runtime::missing_p7_api(AdapterMissingP7Api::SubtitleTrackSwitch);
        case AdapterShortcutAction::AudioTrack:
            return runtime::missing_p7_api(AdapterMissingP7Api::TrackInventory);
    }

    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

AdapterCommandResult PlayerRuntimeAdapter::set_subtitle_size(double scale) noexcept {
    (void)scale;
    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

AdapterCommandResult PlayerRuntimeAdapter::set_subtitle_delay(std::int64_t delay_ms) noexcept {
    (void)delay_ms;
    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

AdapterCommandResult PlayerRuntimeAdapter::set_audio_delay(std::int64_t delay_ms) noexcept {
    (void)delay_ms;
    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

AdapterCommandResult PlayerRuntimeAdapter::set_aspect_mode(AdapterAspectMode mode) noexcept {
    (void)mode;
    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

AdapterCommandResult PlayerRuntimeAdapter::set_tone_mapping_mode(AdapterToneMappingMode mode) noexcept {
    (void)mode;
    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

AdapterCommandResult PlayerRuntimeAdapter::set_playback_speed(double speed) noexcept {
    (void)speed;
    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

AdapterCommandResult PlayerRuntimeAdapter::retry() noexcept {
    playback_graph::RecoveryActionRequest request{};
    request.kind = playback_graph::RecoveryActionKind::Retry;
    return runtime::command_result_from_p7(session_.retry(request));
}

AdapterCommandResult PlayerRuntimeAdapter::reopen() noexcept {
    playback_graph::RecoveryActionRequest request{};
    request.kind = playback_graph::RecoveryActionKind::Reopen;
    return runtime::command_result_from_p7(session_.reopen(request));
}

AdapterCommandResult PlayerRuntimeAdapter::copy_diagnostics() noexcept {
    const auto summary = session_.query_diagnostics_summary();
    if (!summary.valid) {
        return runtime::missing_p7_api(AdapterMissingP7Api::DiagnosticsClipboard);
    }
    AdapterCommandResult result{};
    result.status = AdapterCommandStatus::Accepted;
    return result;
}

AdapterSnapshot PlayerRuntimeAdapter::snapshot() const noexcept {
    return runtime::map_snapshot(session_, rebuffer_count_);
}

}  // namespace kivo::video::player_runtime_adapter
