// audio_evidence_coalescing_policy.hpp — P6A evidence: coalescing policy (§11.11)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceCoalescingPolicy {
    ContractMetadata metadata;
    bool coalesce_same_correlation_same_reason{true};
    int32_t max_coalesced_count{16};
    bool operator==(const AudioEvidenceCoalescingPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
