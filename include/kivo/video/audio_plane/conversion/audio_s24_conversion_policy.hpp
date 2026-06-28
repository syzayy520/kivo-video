// audio_s24_conversion_policy.hpp — P6A conversion: s24 conversion policy (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::conversion {
struct AudioS24ConversionPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool s24in32_to_s24packed_safe{true};
    bool s24packed_to_s24in32_safe{true};
};
}  // namespace kivo::video::audio_plane::conversion
