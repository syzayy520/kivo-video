// p6_capability_registry.hpp — P6A capability: P6 capability registry (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// P6CapabilityRegistry
// Single source of truth for P8 capability claims. Compile-only skeleton.
struct P6CapabilityRegistry {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool is_single_p8_claim_source{true};

    bool operator==(const P6CapabilityRegistry&) const = default;
};

}  // namespace kivo::video::audio_plane::capability
