// audio_description_mix_policy.hpp — P6A mix: audio description (AD) mix policy (§9.3)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct AudioDescriptionMixPolicy {
    ContractMetadata metadata;
    bool p4_may_authorize{true};
    int32_t ad_gain_db{0};
    bool operator==(const AudioDescriptionMixPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
