// apo_enhancement_confidence.hpp — P6A capability: APO confidence (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// ApoEnhancementConfidence
// Confidence that APO enhancements are bypassed or active. Compile-only enum.
enum class ApoEnhancementConfidence {
    Unknown = 0,
    Impossible = 1,
    ProvenBypassed = 2,
    Active = 3
};

}  // namespace kivo::video::audio_plane::capability
