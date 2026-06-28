// audio_evidence_sink_format.hpp — P6A evidence: sink format enum (§11.13)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
enum class EvidenceSinkFormat {
    CompactBinaryRing = 0,
    RedactedJsonLines = 1,
    CompressedJsonL = 2,
    BinaryEventLog = 3
};
}  // namespace kivo::video::audio_plane::evidence
