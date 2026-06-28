// audio_capability_confidence.hpp — P6A capability: confidence grade (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// AudioCapabilityConfidence
// Confidence grade of a capability claim. Compile-only enum.
enum class AudioCapabilityConfidence {
    Unknown = 0,
    Heuristic = 1,
    TrustedApi = 2,
    LabVerified = 3
};

}  // namespace kivo::video::audio_plane::capability
