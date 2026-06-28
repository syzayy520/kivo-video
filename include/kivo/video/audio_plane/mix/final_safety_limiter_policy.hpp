// final_safety_limiter_policy.hpp — P6A mix: final safety limiter policy (§9.12)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct FinalSafetyLimiterPolicy {
    ContractMetadata metadata;
    bool enabled{true};
    double threshold_db{-0.5};
    bool catches_intersample_peaks{true};
    bool operator==(const FinalSafetyLimiterPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
