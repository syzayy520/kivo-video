// loudness_normalization_policy.hpp — P6A mix: loudness normalization policy (§9.8)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct LoudnessNormalizationPolicy {
    ContractMetadata metadata;
    bool enabled{false};
    double target_lufs{-23.0};
    bool breaks_bit_perfect{true};
    bool operator==(const LoudnessNormalizationPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
