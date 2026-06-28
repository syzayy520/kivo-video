// time_stretch_policy.hpp — P6A rate: time-stretch policy (§10.6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::rate {
struct TimeStretchPolicy {
    ContractMetadata metadata;
    bool latency_enters_audible_formula{true};
    bool operator==(const TimeStretchPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::rate
