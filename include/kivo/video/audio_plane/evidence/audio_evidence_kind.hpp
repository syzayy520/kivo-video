// audio_evidence_kind.hpp — P6A evidence: audio evidence kind enum (§11.2)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
enum class AudioEvidenceKind {
    DeviceOpen = 0,
    DeviceClose = 1,
    Underfill = 2,
    DecodeOk = 3,
    DecodeFailed = 4,
    SeekComplete = 5,
    GaplessCommit = 6,
    ClaimBroken = 7,
    FormatChanged = 8,
    ClockDiscontinuity = 9,
    Recovery = 10
};
}  // namespace kivo::video::audio_plane::evidence
