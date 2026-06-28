// audio_user_visible_claim.hpp — P6A capability: user-visible claim (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/capability/audio_capability_state.hpp"
namespace kivo::video::audio_plane::capability {

// AudioUserVisibleClaim
// User-visible surface of a capability claim. Compile-only skeleton.
struct AudioUserVisibleClaim {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int32_t claim_token{0};
    AudioCapabilityState state{AudioCapabilityState::NotClaimable};
    bool is_fresh{false};
    uint64_t claim_generation{0};

    bool operator==(const AudioUserVisibleClaim&) const = default;
};

}  // namespace kivo::video::audio_plane::capability
