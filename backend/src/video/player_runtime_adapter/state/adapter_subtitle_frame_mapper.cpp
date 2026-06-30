#include "video/player_runtime_adapter/state/adapter_subtitle_frame_mapper.hpp"

#include <cstring>

namespace kivo::video::player_runtime_adapter::runtime {
namespace {

void copy_text_field(char* const dst, const std::size_t dst_size, const char* const src) noexcept {
    if (dst == nullptr || dst_size == 0) {
        return;
    }
    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }
    std::strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

}  // namespace

void apply_subtitle_frame(AdapterSnapshot& snapshot,
                          const playback_graph::SubtitleFrameSnapshot& frame) noexcept {
    if (!frame.valid) {
        return;
    }

    snapshot.subtitle_frame_available = frame.frame_available;
    if (frame.frame_available) {
        copy_text_field(snapshot.subtitle_primary, sizeof(snapshot.subtitle_primary),
                        frame.primary.primary_text);
        copy_text_field(snapshot.subtitle_secondary, sizeof(snapshot.subtitle_secondary),
                        frame.secondary.primary_text);
        snapshot.connections.subtitle_frame = AdapterConnectionStatus::ConnectedToP7;
        return;
    }

    snapshot.subtitle_primary[0] = '\0';
    snapshot.subtitle_secondary[0] = '\0';
    if (frame.runtime_connected) {
        snapshot.connections.subtitle_frame = AdapterConnectionStatus::ConnectedToP7;
        return;
    }

    snapshot.connections.subtitle_frame = AdapterConnectionStatus::NotConnectedToP8Runtime;
}

}  // namespace kivo::video::player_runtime_adapter::runtime