#include "video/player_runtime_adapter/state/adapter_subtitle_frame_mapper.hpp"

namespace kivo::video::player_runtime_adapter::runtime {

void apply_subtitle_frame(AdapterSnapshot& snapshot,
                          const playback_graph::SubtitleFrameSnapshot& frame) noexcept {
    if (!frame.valid) {
        return;
    }

    snapshot.subtitle_frame_available = frame.frame_available;
    if (frame.frame_available) {
        snapshot.connections.subtitle_frame = AdapterConnectionStatus::ConnectedToP7;
        return;
    }

    snapshot.connections.subtitle_frame = AdapterConnectionStatus::NotConnectedToP8Runtime;
}

}  // namespace kivo::video::player_runtime_adapter::runtime