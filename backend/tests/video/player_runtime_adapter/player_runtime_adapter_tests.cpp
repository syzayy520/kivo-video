#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

using namespace kivo::video::player_runtime_adapter;
using namespace kivo::video::playback_graph;

namespace {

[[nodiscard]] bool is_missing(const AdapterCommandResult& result,
                              AdapterMissingP7Api missing_api) noexcept {
    return result.status == AdapterCommandStatus::NotConnectedToP7 &&
           result.missing_api == missing_api;
}

[[nodiscard]] bool verify_lifecycle_snapshot() {
    PlayerRuntimeAdapter adapter{};

    const auto invalid_open = adapter.open_media_id(0);
    if (invalid_open.status != AdapterCommandStatus::RejectedByP7 ||
        invalid_open.p7_error != PlaybackGraphError::SourceOpenFailed) {
        return false;
    }

    const auto open = adapter.open_media_id(42);
    if (!open.accepted()) {
        return false;
    }

    const auto ready = adapter.snapshot();
    if (ready.playback_state != RuntimePlaybackState::Ready || ready.is_opening ||
        ready.is_playing || ready.is_buffering || ready.closed) {
        return false;
    }
    if (ready.connections.playback_session != AdapterConnectionStatus::ConnectedToP7 ||
        ready.connections.timeline != AdapterConnectionStatus::ConnectedToP7 ||
        ready.connections.audio_track_switch != AdapterConnectionStatus::ConnectedToP7 ||
        ready.connections.subtitle_tracks != AdapterConnectionStatus::NotConnectedToP7) {
        return false;
    }
    if (ready.duration_ms != 42 * 60000 || ready.position_ms != 0) {
        return false;
    }

    if (!adapter.attach_video_surface(AdapterSurfaceHandle{1}).accepted()) {
        return false;
    }
    const auto with_surface = adapter.snapshot();
    if (with_surface.connections.video_surface != AdapterConnectionStatus::ConnectedToP7 ||
        !with_surface.video_ready) {
        return false;
    }

    if (!adapter.close().accepted()) {
        return false;
    }
    if (!adapter.snapshot().closed) {
        return false;
    }
    return adapter.release_surface().accepted();
}

[[nodiscard]] bool verify_transport_and_timeline_commands() {
    PlayerRuntimeAdapter transport{};
    if (!transport.open_media_id(7).accepted()) {
        return false;
    }
    if (!transport.play().accepted()) {
        return false;
    }
    const auto playing = transport.snapshot();
    if (playing.playback_state != RuntimePlaybackState::Playing || !playing.is_playing) {
        return false;
    }

    PlayerRuntimeAdapter timeline{};
    if (!timeline.open_media_id(8).accepted()) {
        return false;
    }
    if (!timeline.seek(5000).accepted()) {
        return false;
    }
    const auto after_seek = timeline.snapshot();
    if (after_seek.playback_state != RuntimePlaybackState::Playing ||
        after_seek.position_ms != 5000) {
        return false;
    }
    if (after_seek.connections.buffered_ranges != AdapterConnectionStatus::ConnectedToP7) {
        return false;
    }

    PlayerRuntimeAdapter negative_seek{};
    if (!negative_seek.open_media_id(9).accepted()) {
        return false;
    }
    const auto backward = negative_seek.step_backward(10000);
    if (backward.status != AdapterCommandStatus::RejectedByP7 ||
        backward.p7_error != PlaybackGraphError::InvalidSeekTarget) {
        return false;
    }

    return is_missing(negative_seek.stop(), AdapterMissingP7Api::StopWithoutClose);
}

[[nodiscard]] bool verify_track_boundaries() {
    PlayerRuntimeAdapter adapter{};
    if (!adapter.open_media_id(10).accepted()) {
        return false;
    }

    const auto audio = adapter.select_audio_track(2);
    if (audio.status != AdapterCommandStatus::RejectedByP7 ||
        audio.p7_error != PlaybackGraphError::InvalidState) {
        return false;
    }

    const auto video = adapter.select_video_track(3);
    if (video.status != AdapterCommandStatus::RejectedByP7 ||
        video.p7_error != PlaybackGraphError::InvalidState) {
        return false;
    }

    if (!is_missing(adapter.select_subtitle_track(4),
                    AdapterMissingP7Api::SubtitleTrackSwitch)) {
        return false;
    }
    return is_missing(adapter.disable_subtitle(), AdapterMissingP7Api::SubtitleTrackSwitch);
}

[[nodiscard]] bool verify_shortcut_name_bridge() {
    PlayerRuntimeAdapter adapter{};
    if (!adapter.open_media_id(12).accepted()) {
        return false;
    }
    if (!adapter.handle_shortcut_name("togglePlayPause").accepted()) {
        return false;
    }
    if (!adapter.handle_shortcut_name("playPause").accepted()) {
        return false;
    }
    if (!is_missing(adapter.handle_shortcut_name("subtitle"),
                    AdapterMissingP7Api::SubtitleTrackSwitch)) {
        return false;
    }
    return is_missing(adapter.handle_shortcut_name("audioTrack"),
                      AdapterMissingP7Api::TrackInventory);
}

[[nodiscard]] bool verify_missing_p7_connection_groups() {
    PlayerRuntimeAdapter adapter{};
    if (!adapter.open_media_id(11).accepted()) {
        return false;
    }

    if (!is_missing(adapter.enter_fullscreen(), AdapterMissingP7Api::FullscreenWindowScreen) ||
        !is_missing(adapter.exit_fullscreen(), AdapterMissingP7Api::FullscreenWindowScreen) ||
        !is_missing(adapter.move_to_screen(1), AdapterMissingP7Api::FullscreenWindowScreen)) {
        return false;
    }
    if (!is_missing(adapter.next(), AdapterMissingP7Api::Playlist) ||
        !is_missing(adapter.previous(), AdapterMissingP7Api::Playlist) ||
        !is_missing(adapter.play_next_episode(), AdapterMissingP7Api::Playlist) ||
        !is_missing(adapter.set_auto_play_next(true), AdapterMissingP7Api::Playlist)) {
        return false;
    }
    if (!adapter.handle_shortcut(AdapterShortcutAction::TogglePlayPause).accepted()) {
        return false;
    }
    if (!is_missing(adapter.handle_shortcut(AdapterShortcutAction::VolumeUp),
                    AdapterMissingP7Api::AudioVolume) ||
        !is_missing(adapter.handle_shortcut(AdapterShortcutAction::ToggleFullscreen),
                    AdapterMissingP7Api::FullscreenWindowScreen) ||
        !is_missing(adapter.handle_shortcut(AdapterShortcutAction::Subtitle),
                    AdapterMissingP7Api::SubtitleTrackSwitch) ||
        !is_missing(adapter.handle_shortcut(AdapterShortcutAction::AudioTrack),
                    AdapterMissingP7Api::TrackInventory)) {
        return false;
    }
    if (!is_missing(adapter.set_subtitle_size(1.1), AdapterMissingP7Api::UserSettingsPolicy) ||
        !is_missing(adapter.set_subtitle_delay(120), AdapterMissingP7Api::UserSettingsPolicy) ||
        !is_missing(adapter.set_audio_delay(-80), AdapterMissingP7Api::UserSettingsPolicy) ||
        !is_missing(adapter.set_aspect_mode(AdapterAspectMode::Fit),
                    AdapterMissingP7Api::UserSettingsPolicy) ||
        !is_missing(adapter.set_tone_mapping_mode(AdapterToneMappingMode::Auto),
                    AdapterMissingP7Api::UserSettingsPolicy) ||
        !is_missing(adapter.set_playback_speed(1.25),
                    AdapterMissingP7Api::UserSettingsPolicy)) {
        return false;
    }
    if (!is_missing(adapter.set_volume(0.5), AdapterMissingP7Api::AudioVolume) ||
        !is_missing(adapter.set_muted(true), AdapterMissingP7Api::AudioVolume) ||
        !is_missing(adapter.select_audio_device("default"), AdapterMissingP7Api::AudioDevice)) {
        return false;
    }
    if (!adapter.reopen().accepted()) {
        return false;
    }
    if (!adapter.copy_diagnostics().accepted()) {
        return false;
    }

    const auto retry_result = adapter.retry();
    return retry_result.status == AdapterCommandStatus::RejectedByP7 &&
           retry_result.p7_error == PlaybackGraphError::InvalidState;
}

}  // namespace

int main() {
    if (!verify_lifecycle_snapshot()) {
        return 1;
    }
    if (!verify_transport_and_timeline_commands()) {
        return 2;
    }
    if (!verify_track_boundaries()) {
        return 3;
    }
    if (!verify_shortcut_name_bridge()) {
        return 4;
    }
    if (!verify_missing_p7_connection_groups()) {
        return 5;
    }
    return 0;
}