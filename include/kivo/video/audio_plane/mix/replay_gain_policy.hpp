// replay_gain_policy.hpp — P6A mix: ReplayGain policy (§9.7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct ReplayGainPolicy {
    ContractMetadata metadata;
    bool enabled{false};
    double gain_db{0.0};
    bool breaks_bit_perfect{true};
    bool operator==(const ReplayGainPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
