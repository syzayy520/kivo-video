// audio_channel_mask.hpp — P6A channel: audio channel mask (§8.3)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::channel {
struct AudioChannelMask {
    uint32_t mask{0};
    bool operator==(const AudioChannelMask&) const = default;
};
}  // namespace kivo::video::audio_plane::channel
