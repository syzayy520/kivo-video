// audio_limiter_policy.hpp — P6A mix: audio pre-limiter policy (§9.11)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct AudioLimiterPolicy {
    ContractMetadata metadata;
    bool pre_limiter{false};
    double threshold_db{0.0};
    bool operator==(const AudioLimiterPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
