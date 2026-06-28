// wave_channel_mask_mapping_boundary.hpp — P6A channel: WAVE channel-mask mapping boundary (§8.5)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::channel {
struct WaveChannelMaskMappingBoundary {
    ContractMetadata metadata;
    uint32_t wave_mask{0};
    int32_t canonical_layout{0};
    bool is_boundary_only{true};
    bool operator==(const WaveChannelMaskMappingBoundary&) const = default;
};
}  // namespace kivo::video::audio_plane::channel
