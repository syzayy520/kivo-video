// audio_quantization_policy.hpp — P6A conversion: quantization policy (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::conversion {
struct AudioQuantizationPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    int32_t rounding_mode{0};
    bool dither_required_for_bit_reduction{true};
};
}  // namespace kivo::video::audio_plane::conversion
