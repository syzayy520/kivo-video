// pitch_preserve_policy.hpp — P6A rate: pitch preserve policy (§10.7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::rate {
struct PitchPreservePolicy {
    ContractMetadata metadata;
    bool enabled{true};
    bool operator==(const PitchPreservePolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::rate
