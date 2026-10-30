#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

CommandToken PlaybackSessionRuntime::enter_fullscreen() noexcept {
    const auto token = accept_command(GraphCommandKind::SetWindowScreenPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    window_screen_policy_.mode = PlaybackWindowMode::Fullscreen;
    window_screen_policy_.valid = true;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::exit_fullscreen() noexcept {
    const auto token = accept_command(GraphCommandKind::SetWindowScreenPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    window_screen_policy_.mode = PlaybackWindowMode::Windowed;
    window_screen_policy_.valid = true;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::move_to_screen(
    const ScreenMoveRequest& request) noexcept {
    if (request.screen_index < 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    const auto token = accept_command(GraphCommandKind::SetWindowScreenPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    window_screen_policy_.screen_index = request.screen_index;
    window_screen_policy_.screen_index_valid = true;
    window_screen_policy_.valid = true;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

WindowScreenPolicySnapshot PlaybackSessionRuntime::query_window_screen_policy()
    const noexcept {
    return window_screen_policy_;
}

}  // namespace kivo::video::playback_graph::runtime
