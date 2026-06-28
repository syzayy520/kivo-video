// audio_channel_layout.hpp — P6A channel: audio channel layout (§8.2)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::channel {
struct AudioChannelLayout {
    ContractMetadata metadata;
    int32_t channel_count{0};
    int32_t layout_id{0};
    bool is_unknown{false};
    bool is_dual_mono{false};
    bool operator==(const AudioChannelLayout&) const = default;
};
}  // namespace kivo::video::audio_plane::channel
