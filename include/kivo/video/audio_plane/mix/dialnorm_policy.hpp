// dialnorm_policy.hpp — P6A mix: dialnorm (dialog normalization) policy (§9.6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct DialnormPolicy {
    ContractMetadata metadata;
    bool enabled{false};
    int32_t metadata_source{0};
    bool applying_breaks_bit_perfect{true};
    bool operator==(const DialnormPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
