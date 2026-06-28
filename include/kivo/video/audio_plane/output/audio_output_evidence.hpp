// audio_output_evidence.hpp — P6A §32: output evidence record
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
enum class OutputEvidenceKind : int32_t { Opened = 0, Closed = 1, Underfill = 2, SilenceFill = 3, DeviceLost = 4, Recovered = 5, FormatChanged = 6, ClaimBroken = 7 };
struct AudioOutputEvidence {
    ContractMetadata metadata;
    OutputEvidenceKind kind{OutputEvidenceKind::Opened};
    uint64_t evidence_id{0};
    uint64_t correlation_id{0};
    int64_t timestamp_ms{0};
    int32_t detail{0};
};
}  // namespace kivo::video::audio_plane::output
