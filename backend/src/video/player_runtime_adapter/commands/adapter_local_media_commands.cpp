#include <string>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "video/playback_graph/local_media/local_media_source_registry.hpp"
#include "video/player_runtime_adapter/commands/adapter_p7_request_mappers.hpp"
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
    const std::uint64_t source_id =
        playback_graph::local_media::LocalMediaSourceRegistry::instance().register_path(
            std::string(path));
    AdapterOpenRequest request{};
    request.source_id = source_id;
    return runtime::command_result_from_p7(
        session_.open(commands::to_p7_open_request(request)));
}

playback_graph::LocalMediaPlaybackQuery PlayerRuntimeAdapter::query_local_media_playback()
    const noexcept {
    return session_.query_local_media_playback();
}

int PlayerRuntimeAdapter::pump_local_media_playback(const int max_packets) noexcept {
    return session_.pump_local_media_playback(max_packets);
}

}  // namespace kivo::video::player_runtime_adapter