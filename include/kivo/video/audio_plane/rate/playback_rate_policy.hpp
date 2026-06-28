// playback_rate_policy.hpp — P6A rate: playback rate policy (§10.1)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::rate {
struct PlaybackRatePolicy {
    ContractMetadata metadata;
    double min_rate{0.25};
    double max_rate{4.0};
    bool outside_range_rejected{true};
    bool operator==(const PlaybackRatePolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::rate
