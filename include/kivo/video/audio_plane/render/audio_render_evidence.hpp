// audio_render_evidence.hpp — P6A §32: render evidence
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::render {
enum class RenderEvidenceKind { Underfill=0, SilenceFill=1, StaleDiscard=2, RemainderOverwrite=3, ClaimBroken=4, DeviceLost=5 };
struct AudioRenderEvidence {
    ContractMetadata metadata;
    RenderEvidenceKind kind{RenderEvidenceKind::Underfill};
    uint64_t evidence_id{0};
    uint64_t correlation_id{0};
    int64_t timestamp_ms{0};
};
}  // namespace kivo::video::audio_plane::render
