// audio_evidence_capacity_policy.hpp — P6A evidence: capacity policy (§11.9)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceCapacityPolicy {
    ContractMetadata metadata;
    int32_t hot_ring_capacity{1024};
    int32_t disk_queue_capacity{10000};
    bool operator==(const AudioEvidenceCapacityPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
