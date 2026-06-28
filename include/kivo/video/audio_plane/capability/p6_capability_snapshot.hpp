// p6_capability_snapshot.hpp — P6A capability: P6 capability snapshot (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// P6CapabilitySnapshot
// Snapshot of the current capability claim state. output_mode and claim_grade
// are enum-as-int placeholders. claim_generation ties snapshot to claim epoch.
struct P6CapabilitySnapshot {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int32_t output_mode{0};
    int32_t claim_grade{0};
    uint64_t claim_generation{0};

    bool operator==(const P6CapabilitySnapshot&) const = default;
};

}  // namespace kivo::video::audio_plane::capability
