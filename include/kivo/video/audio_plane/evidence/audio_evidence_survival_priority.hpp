// audio_evidence_survival_priority.hpp — P6A evidence: survival priority (§11.7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
enum class EvidenceSurvivalPriority {
    Drop = 0,
    Coalesce = 1,
    Critical = 2
};
struct AudioEvidenceSurvivalPriority {
    ContractMetadata metadata;
    EvidenceSurvivalPriority priority{EvidenceSurvivalPriority::Drop};
    bool operator==(const AudioEvidenceSurvivalPriority&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
