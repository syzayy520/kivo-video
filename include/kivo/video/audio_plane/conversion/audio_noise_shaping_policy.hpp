// audio_noise_shaping_policy.hpp — P6A conversion: noise shaping policy (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::conversion {
struct AudioNoiseShapingPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool enabled{false};
    int32_t order{0};
};
}  // namespace kivo::video::audio_plane::conversion
