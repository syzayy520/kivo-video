#include <string>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "video/player_runtime_adapter/p7_bridge/p7_command_result.hpp"

namespace kivo::video::player_runtime_adapter {

AdapterCommandResult PlayerRuntimeAdapter::open_local_media_file(
    const std::string_view path) noexcept {
    if (path.empty()) {
        AdapterCommandResult result{};
        result.status = AdapterCommandStatus::RejectedByP7;
        result.p7_error = playback_graph::PlaybackGraphError::SourceOpenFailed;
        return result;
    }
    return runtime::command_result_from_p7(session_.open_local_media_file(path));
}

playback_graph::LocalMediaPlaybackQuery PlayerRuntimeAdapter::query_local_media_playback(
    const bool include_frame_pixels) const noexcept {
    return session_.query_local_media_playback(include_frame_pixels);
}

int PlayerRuntimeAdapter::pump_local_media_playback(const int max_packets) noexcept {
    return session_.pump_local_media_playback(max_packets);
}

}  // namespace kivo::video::player_runtime_adapter
