// rate_change_boundary_policy.hpp — P6A rate: rate change boundary policy (§10.8)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::rate {
struct RateChangeBoundaryPolicy {
    ContractMetadata metadata;
    bool rate_not_1_disables_bit_perfect{true};
    bool passthrough_rejected_unless_verified_encoded_rate{true};
    bool operator==(const RateChangeBoundaryPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::rate
