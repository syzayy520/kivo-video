// audio_container_layout_mapping.hpp — P6A channel: container-to-canonical layout mapping (§8.4)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::channel {
struct AudioContainerLayoutMapping {
    ContractMetadata metadata;
    int32_t container_layout{0};
    int32_t canonical_layout{0};
    bool mapping_required{true};
    bool operator==(const AudioContainerLayoutMapping&) const = default;
};
}  // namespace kivo::video::audio_plane::channel
