// gapless_evidence.hpp — P6A gapless: gapless evidence contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
enum class GaplessEvidenceKind {
    PrepareOk = 0,
    CancelReserve = 1,
    CommitOk = 2,
    ReserveMissed = 3,
    SampleContinuityOk = 4
};
struct GaplessEvidence {
    kivo::video::audio_plane::ContractMetadata metadata;
    GaplessEvidenceKind kind;
    uint64_t evidence_id{0};
};
}  // namespace kivo::video::audio_plane::gapless
