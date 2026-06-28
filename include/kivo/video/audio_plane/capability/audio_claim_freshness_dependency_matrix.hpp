// audio_claim_freshness_dependency_matrix.hpp — P6A capability: freshness matrix (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// AudioClaimFreshnessDependencyMatrix
// Declares which generations the claim freshness depends on. Compile-only.
struct AudioClaimFreshnessDependencyMatrix {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool engine_epoch{true};
    bool device_generation{true};
    bool stream_generation{true};
    bool format_generation{true};
    bool output_mode_generation{true};
    bool volume_policy_generation{true};
    bool software_path_generation{true};
    bool ducking_generation{true};
    bool apo_enhancement_generation{true};
    bool passthrough_policy_generation{true};
    bool bitperfect_policy_generation{true};
    bool license_generation{true};
    bool claim_generation{true};
    bool evidence_ttl{true};

    bool operator==(const AudioClaimFreshnessDependencyMatrix&) const = default;
};

}  // namespace kivo::video::audio_plane::capability
