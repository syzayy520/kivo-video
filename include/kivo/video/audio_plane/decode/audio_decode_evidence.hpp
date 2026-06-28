// audio_decode_evidence.hpp — P6A decode: DecodeEvidenceKind + AudioDecodeEvidence (§7.9)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::decode {
enum class DecodeEvidenceKind {
    InitOk = 0,
    InitFailed = 1,
    DecodeOk = 2,
    DecodeFailed = 3,
    TimedOut = 4,
    Flushed = 5,
    Drained = 6,
    FormatChanged = 7,
    Superseded = 8,
};
struct AudioDecodeEvidence {
    ContractMetadata metadata;
    DecodeEvidenceKind kind{DecodeEvidenceKind::InitOk};
    uint64_t evidence_id{0};
    uint64_t correlation_id{0};
    bool operator==(const AudioDecodeEvidence&) const = default;
};
}  // namespace kivo::video::audio_plane::decode
