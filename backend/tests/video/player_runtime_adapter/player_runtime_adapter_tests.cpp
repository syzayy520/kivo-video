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
        ready.connections.track_inventory != AdapterConnectionStatus::ConnectedToP7 ||
        ready.connections.subtitle_tracks != AdapterConnectionStatus::ConnectedToP7 ||
        ready.connections.subtitle_frame != AdapterConnectionStatus::NotConnectedToP8Runtime ||
        ready.connections.audio_volume != AdapterConnectionStatus::ConnectedToP7 ||
        ready.connections.user_settings_policy != AdapterConnectionStatus::ConnectedToP7) {
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

[[nodiscard]] bool verify_transport_stop_and_timeline() {
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

    if (!transport.stop().accepted()) {
        return false;
    }
    const auto stopped = transport.snapshot();
    if (stopped.playback_state != RuntimePlaybackState::Ready || stopped.is_playing) {
        return false;
    }
    if (!transport.play().accepted()) {
        return false;
    }

    if (!transport.close().accepted()) {
        return false;
    }
    const auto closed = transport.snapshot();
    if (!closed.closed || closed.playback_state != RuntimePlaybackState::Closed) {
        return false;
    }
    const auto play_after_close = transport.play();
    if (play_after_close.status != AdapterCommandStatus::RejectedByP7) {
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
    return backward.status == AdapterCommandStatus::RejectedByP7 &&
           backward.p7_error == PlaybackGraphError::InvalidSeekTarget;
}

[[nodiscard]] bool verify_subtitle_and_audio_p0() {
    PlayerRuntimeAdapter adapter{};
    if (!adapter.open_media_id(10).accepted() || !adapter.play().accepted()) {
        return false;
    }

    if (!adapter.select_subtitle_track(4).accepted()) {
        return false;
    }
    const auto with_subtitle = adapter.snapshot();
    if (!with_subtitle.subtitle_enabled || with_subtitle.selected_subtitle_track != 4 ||
        with_subtitle.connections.subtitle_tracks != AdapterConnectionStatus::ConnectedToP7 ||
        with_subtitle.connections.subtitle_frame != AdapterConnectionStatus::ConnectedToP7 ||
        !with_subtitle.subtitle_frame_available ||
        with_subtitle.subtitle_primary[0] != 'c') {
        return false;
    }

    if (!adapter.set_subtitle_delay(120).accepted()) {
        return false;
    }
    if (adapter.snapshot().subtitle_delay_ms != 120) {
        return false;
    }

    if (!adapter.disable_subtitle().accepted()) {
        return false;
    }
    if (adapter.snapshot().subtitle_enabled) {
        return false;
    }

    if (!adapter.set_volume(0.5).accepted() || !adapter.set_muted(true).accepted()) {
        return false;
    }
    const auto audio = adapter.snapshot();
    if (audio.volume != 0.5 || !audio.muted ||
        audio.connections.audio_volume != AdapterConnectionStatus::ConnectedToP7) {
        return false;
    }

    if (!adapter.select_audio_device("default").accepted()) {
        return false;
    }
    if (adapter.snapshot().connections.audio_device != AdapterConnectionStatus::ConnectedToP7) {
        return false;
    }

    return adapter.set_audio_delay(-80).accepted() &&
           adapter.snapshot().audio_delay_ms == -80;
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

    const auto subtitle = adapter.select_subtitle_track(4);
    return subtitle.status == AdapterCommandStatus::RejectedByP7 &&
           subtitle.p7_error == PlaybackGraphError::InvalidState;
}

[[nodiscard]] bool verify_settings_policy_p1() {
    PlayerRuntimeAdapter adapter{};
    if (!adapter.open_media_id(13).accepted()) {
        return false;
    }

    if (!adapter.set_aspect_mode(AdapterAspectMode::Fit).accepted() ||
        !adapter.set_scale_mode(AdapterScaleMode::Fill).accepted() ||
        !adapter.set_tone_mapping_mode(AdapterToneMappingMode::Auto).accepted() ||
        !adapter.set_deinterlace_mode(AdapterDeinterlaceMode::Off).accepted() ||
        !adapter.set_playback_speed(1.25).accepted() ||
        !adapter.set_subtitle_size(1.1).accepted()) {
        return false;
    }

    const auto settings = adapter.snapshot();
    return settings.connections.user_settings_policy == AdapterConnectionStatus::ConnectedToP7 &&
           settings.aspect_mode == AdapterAspectMode::Fit &&
           settings.scale_mode == AdapterScaleMode::Fill &&
           settings.tone_mapping_mode == AdapterToneMappingMode::Auto &&
           settings.deinterlace_mode == AdapterDeinterlaceMode::Off &&
           settings.playback_speed == 1.25 && settings.subtitle_size == 1.1;
}

[[nodiscard]] bool verify_shortcut_name_bridge() {
    PlayerRuntimeAdapter adapter{};
    if (!adapter.open_media_id(12).accepted() || !adapter.play().accepted()) {
        return false;
    }
    if (!adapter.select_subtitle_track(1).accepted()) {
        return false;
    }
    if (!adapter.handle_shortcut_name("togglePlayPause").accepted()) {
        return false;
    }
    if (!adapter.handle_shortcut_name("volumeUp").accepted()) {
        return false;
    }
    if (!adapter.handle_shortcut_name("subtitle").accepted()) {
        return false;
    }
    if (adapter.snapshot().subtitle_enabled) {
        return false;
    }
    return adapter.handle_shortcut_name("audioTrack").accepted();
}

[[nodiscard]] bool verify_track_inventory_and_diagnostics_closure() {
    PlayerRuntimeAdapter adapter{};
    if (!adapter.open_media_id(15).accepted() || !adapter.play().accepted()) {
        return false;
    }

    const auto inventory = adapter.snapshot();
    if (inventory.connections.track_inventory != AdapterConnectionStatus::ConnectedToP7 ||
        inventory.selected_audio_track != 1 ||
        inventory.connections.subtitle_frame != AdapterConnectionStatus::NotConnectedToP8Runtime ||
        inventory.subtitle_frame_available) {
        return false;
    }

    if (!adapter.handle_shortcut(AdapterShortcutAction::Subtitle).accepted()) {
        return false;
    }
    const auto with_subtitle = adapter.snapshot();
    if (!with_subtitle.subtitle_enabled ||
        with_subtitle.connections.subtitle_frame != AdapterConnectionStatus::ConnectedToP7) {
        return false;
    }

    if (!adapter.copy_diagnostics().accepted()) {
        return false;
    }
    if (!adapter.close().accepted()) {
        return false;
    }

    const auto invalid_copy = adapter.copy_diagnostics();
    return invalid_copy.status == AdapterCommandStatus::RejectedByP7 &&
           invalid_copy.p7_error == PlaybackGraphError::ClosedGraph;
}

[[nodiscard]] bool verify_remaining_not_connected() {
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
    if (!is_missing(adapter.handle_shortcut(AdapterShortcutAction::ToggleFullscreen),
                    AdapterMissingP7Api::FullscreenWindowScreen)) {
        return false;
    }
    if (!adapter.reopen().accepted() || !adapter.copy_diagnostics().accepted()) {
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
    if (!verify_transport_stop_and_timeline()) {
        return 2;
    }
    if (!verify_subtitle_and_audio_p0()) {
        return 3;
    }
    if (!verify_track_boundaries()) {
        return 4;
    }
    if (!verify_settings_policy_p1()) {
        return 5;
    }
    if (!verify_shortcut_name_bridge()) {
        return 6;
    }
    if (!verify_track_inventory_and_diagnostics_closure()) {
        return 7;
    }
    if (!verify_remaining_not_connected()) {
        return 8;
    }
    return 0;
}