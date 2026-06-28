// audio_capability_claim_mapping.hpp — P6A capability: claim mapping (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/capability/audio_capability_state.hpp"
namespace kivo::video::audio_plane::capability {

// AudioCapabilityClaimMapping
// Maps a capability_id to its claim_token and current state.
struct AudioCapabilityClaimMapping {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int32_t capability_id{0};
    int32_t claim_token{0};
    AudioCapabilityState state{AudioCapabilityState::NotClaimable};

    bool operator==(const AudioCapabilityClaimMapping&) const = default;
};

}  // namespace kivo::video::audio_plane::capability
