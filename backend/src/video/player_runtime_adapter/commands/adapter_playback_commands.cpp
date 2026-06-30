#include <algorithm>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "video/player_runtime_adapter/p7_bridge/p7_command_result.hpp"

namespace kivo::video::player_runtime_adapter {

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

AdapterCommandResult PlayerRuntimeAdapter::handle_shortcut_name(const std::string_view name) noexcept {
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

AdapterCommandResult PlayerRuntimeAdapter::handle_shortcut(
    const AdapterShortcutAction action) noexcept {
    switch (action) {
        case AdapterShortcutAction::TogglePlayPause:
            return toggle_play_pause();
        case AdapterShortcutAction::SeekBackward:
            return step_backward(10000);
        case AdapterShortcutAction::SeekForward:
            return step_forward(10000);
        case AdapterShortcutAction::VolumeUp: {
            const auto current = snapshot();
            return set_volume(std::min(1.0, current.volume + 0.1));
        }
        case AdapterShortcutAction::VolumeDown: {
            const auto current = snapshot();
            return set_volume(std::max(0.0, current.volume - 0.1));
        }
        case AdapterShortcutAction::ToggleMute:
            return set_muted(!snapshot().muted);
        case AdapterShortcutAction::ToggleFullscreen:
        case AdapterShortcutAction::ExitFullscreen:
            return runtime::missing_p7_api(AdapterMissingP7Api::FullscreenWindowScreen);
        case AdapterShortcutAction::Subtitle:
            if (snapshot().subtitle_enabled) {
                return disable_subtitle();
            }
            return runtime::command_result_from_p7(session_.cycle_subtitle_track());
        case AdapterShortcutAction::AudioTrack:
            return runtime::command_result_from_p7(session_.cycle_audio_track());
    }

    return runtime::missing_p7_api(AdapterMissingP7Api::UserSettingsPolicy);
}

}  // namespace kivo::video::player_runtime_adapter