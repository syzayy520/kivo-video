// audio_quality_golden_test_contract.hpp — P6A test_contract: golden (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioQualityGoldenTestContract
// Contract for quality golden tests. Compile-only skeleton.
struct AudioQualityGoldenTestContract {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool bit_exact_pcm_null{true};
    bool valid_bits_preservation{true};
    bool s24_conversion{true};
    bool dither_spectrum{true};
    bool resampler_thdn{true};
    bool channel_mapping_impulse{true};
    bool downmix_golden_vector{true};
    bool gapless_sample_boundary{true};

    bool operator==(const AudioQualityGoldenTestContract&) const = default;
};

}  // namespace kivo::video::audio_plane::test_contract
