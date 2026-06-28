// passthrough_evidence.hpp — P6A passthrough: PassthroughEvidenceKind + PassthroughEvidence (§8.10)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
enum class PassthroughEvidenceKind {
    SyncFrameEstablished = 0,
    Underfill = 1,
    Rebuffer = 2,
    BurstMappingLost = 3,
    ClaimBroken = 4,
};
struct PassthroughEvidence {
    ContractMetadata metadata;
    PassthroughEvidenceKind kind{PassthroughEvidenceKind::SyncFrameEstablished};
    uint64_t evidence_id{0};
    int64_t timestamp_ms{0};
    bool operator==(const PassthroughEvidence&) const = default;
};
}  // namespace kivo::video::audio_plane::passthrough
