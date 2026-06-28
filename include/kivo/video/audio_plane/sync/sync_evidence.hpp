// sync_evidence.hpp — P6A sync: SyncEvidence
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
struct SyncEvidence {
    ContractMetadata metadata;
    uint64_t evidence_id{0};
    int32_t kind{0};
    int64_t drift_ms{0};
    int64_t timestamp_ms{0};
    bool operator==(const SyncEvidence&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
