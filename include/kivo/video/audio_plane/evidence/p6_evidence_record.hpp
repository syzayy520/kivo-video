// p6_evidence_record.hpp — P6A evidence: P6 evidence record (§11.1)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct P6EvidenceRecord {
    ContractMetadata metadata;
    uint64_t evidence_id{0};
    uint64_t correlation_id{0};
    int32_t origin{0};
    int32_t kind{0};
    int64_t timestamp_ms{0};
    int32_t severity{0};
    uint64_t engine_epoch{0};
    bool operator==(const P6EvidenceRecord&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
