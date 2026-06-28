// audio_sample_conversion_policy.hpp — P6A conversion: sample conversion policy (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::conversion {
struct AudioSampleConversionPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool dither_before_quantization{true};
    bool limiter_after_gain{true};
    bool final_safety_limiter_after_resampler{true};
    bool bit_perfect_disables_mutating_stages{true};
};
}  // namespace kivo::video::audio_plane::conversion
