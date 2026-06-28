// audio_capability_state.hpp — P6A capability: audio capability state (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// AudioCapabilityState
// Lifecycle state of a capability claim. Compile-only enum.
enum class AudioCapabilityState {
    NotClaimable = 0,
    Eligible = 1,
    ActiveUnverified = 2,
    ActiveMeasured = 3,
    ActiveVerified = 4,
    BrokenAfterStart = 5
};

}  // namespace kivo::video::audio_plane::capability
