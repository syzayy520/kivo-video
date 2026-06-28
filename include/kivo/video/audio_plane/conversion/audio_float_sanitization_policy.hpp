// audio_float_sanitization_policy.hpp — P6A conversion: float sanitization policy (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::conversion {
struct AudioFloatSanitizationPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool nan_to_zero{true};
    bool inf_to_clamp{true};
    bool sanitization_breaks_bit_perfect{true};
};
}  // namespace kivo::video::audio_plane::conversion
