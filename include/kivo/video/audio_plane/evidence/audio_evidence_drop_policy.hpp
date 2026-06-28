// audio_evidence_drop_policy.hpp — P6A evidence: drop policy (§11.10)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceDropPolicy {
    ContractMetadata metadata;
    bool chain_root_never_dropped{true};
    bool if_full_increment_critical_counter{true};
    bool coalesce_if_same_correlation_and_reason{true};
    bool emit_storage_degradation_evidence{true};
    bool operator==(const AudioEvidenceDropPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
