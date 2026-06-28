// audio_evidence_storage_policy.hpp — P6A evidence: storage policy (§11.14)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/evidence/audio_evidence_sink_format.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceStoragePolicy {
    ContractMetadata metadata;
    EvidenceSinkFormat hot_format{EvidenceSinkFormat::CompactBinaryRing};
    EvidenceSinkFormat debug_export_format{EvidenceSinkFormat::RedactedJsonLines};
    bool disk_full_triggers_degradation{true};
    bool operator==(const AudioEvidenceStoragePolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
