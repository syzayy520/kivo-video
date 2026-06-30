#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "kivo/video/playback_graph/track_switch_request.hpp"
#include "video/player_runtime_adapter/p7_bridge/p7_command_result.hpp"

namespace kivo::video::player_runtime_adapter {

AdapterCommandResult PlayerRuntimeAdapter::select_video_track(const std::uint64_t track_id) noexcept {
    playback_graph::VideoTrackSwitchRequest request{};
    request.track_id = track_id;
    return runtime::command_result_from_p7(session_.switch_video_track(request));
}

AdapterCommandResult PlayerRuntimeAdapter::select_audio_track(const std::uint64_t track_id) noexcept {
    playback_graph::AudioTrackSwitchRequest request{};
    request.track_id = track_id;
    return runtime::command_result_from_p7(session_.switch_audio_track(request));
}

AdapterCommandResult PlayerRuntimeAdapter::select_subtitle_track(
    const std::uint64_t track_id) noexcept {
    playback_graph::SubtitleTrackSwitchRequest request{};
    request.track_id = track_id;
    return runtime::command_result_from_p7(session_.switch_subtitle_track(request));
}

AdapterCommandResult PlayerRuntimeAdapter::disable_subtitle() noexcept {
    return runtime::command_result_from_p7(session_.disable_subtitle());
}

}  // namespace kivo::video::player_runtime_adapter