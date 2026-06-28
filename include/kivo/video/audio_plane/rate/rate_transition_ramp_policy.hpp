// rate_transition_ramp_policy.hpp — P6A rate: rate transition ramp policy (§10.5)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::rate {
struct RateTransitionRampPolicy {
    ContractMetadata metadata;
    int32_t ramp_window_ms{10};
    bool required_to_avoid_click{true};
    bool operator==(const RateTransitionRampPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::rate
