#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "video/player_runtime_adapter/commands/adapter_p7_request_mappers.hpp"
#include "video/player_runtime_adapter/p7_bridge/p7_command_result.hpp"

namespace kivo::video::player_runtime_adapter {

AdapterCommandResult PlayerRuntimeAdapter::open(const AdapterOpenRequest& request) noexcept {
    return runtime::command_result_from_p7(
        session_.open(commands::to_p7_open_request(request)));
}

AdapterCommandResult PlayerRuntimeAdapter::open_media_id(const std::uint64_t media_id) noexcept {
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

AdapterCommandResult PlayerRuntimeAdapter::attach_video_surface(
    const AdapterSurfaceHandle handle) noexcept {
    playback_graph::VideoSurfaceBindingRequest request{};
    request.native_handle = handle.native_handle;
    return runtime::command_result_from_p7(session_.attach_video_surface(request));
}

AdapterCommandResult PlayerRuntimeAdapter::stop() noexcept {
    return runtime::command_result_from_p7(session_.stop());
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

AdapterCommandResult PlayerRuntimeAdapter::enter_fullscreen() noexcept {
    return runtime::command_result_from_p7(session_.enter_fullscreen());
}

AdapterCommandResult PlayerRuntimeAdapter::exit_fullscreen() noexcept {
    return runtime::command_result_from_p7(session_.exit_fullscreen());
}

AdapterCommandResult PlayerRuntimeAdapter::move_to_screen(const std::int32_t screen_index) noexcept {
    playback_graph::ScreenMoveRequest request{};
    request.screen_index = screen_index;
    return runtime::command_result_from_p7(session_.move_to_screen(request));
}

AdapterCommandResult PlayerRuntimeAdapter::next() noexcept {
    return runtime::command_result_from_p7(session_.next());
}

AdapterCommandResult PlayerRuntimeAdapter::previous() noexcept {
    return runtime::command_result_from_p7(session_.previous());
}

AdapterCommandResult PlayerRuntimeAdapter::play_next_episode() noexcept {
    return runtime::command_result_from_p7(session_.play_next_episode());
}

AdapterCommandResult PlayerRuntimeAdapter::play_playlist_item(
    const std::uint64_t index) noexcept {
    playback_graph::PlaylistItemSelectRequest request{};
    request.index = index;
    return runtime::command_result_from_p7(session_.play_playlist_item(request));
}

AdapterCommandResult PlayerRuntimeAdapter::set_auto_play_next(const bool enabled) noexcept {
    playback_graph::AutoPlayNextRequest request{};
    request.enabled = enabled;
    return runtime::command_result_from_p7(session_.set_auto_play_next(request));
}

}  // namespace kivo::video::player_runtime_adapter
