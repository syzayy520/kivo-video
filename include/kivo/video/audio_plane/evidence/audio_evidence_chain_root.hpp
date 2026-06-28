// audio_evidence_chain_root.hpp — P6A evidence: chain root policy (§11.8)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceChainRoot {
    ContractMetadata metadata;
    uint64_t root_evidence_id{0};
    bool cannot_be_dropped_silently{true};
    bool operator==(const AudioEvidenceChainRoot&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
