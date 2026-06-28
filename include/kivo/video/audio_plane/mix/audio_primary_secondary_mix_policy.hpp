// audio_primary_secondary_mix_policy.hpp — P6A mix: primary/secondary mix policy (§9.2)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct AudioPrimarySecondaryMixPolicy {
    ContractMetadata metadata;
    bool p4_may_authorize{true};
    bool secondary_timestamp_aligned{true};
    bool per_track_src{true};
    bool operator==(const AudioPrimarySecondaryMixPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
