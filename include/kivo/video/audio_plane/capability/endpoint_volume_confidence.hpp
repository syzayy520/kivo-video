// endpoint_volume_confidence.hpp — P6A capability: endpoint volume confidence (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// EndpointVolumeConfidence
// Confidence grade of the endpoint volume claim. Compile-only enum.
enum class EndpointVolumeConfidence {
    Unknown = 0,
    SoftwarePossiblyHeuristic = 1,
    TrustedEndpointApi = 2,
    LabVerifiedHardwareVolume = 3
};

}  // namespace kivo::video::audio_plane::capability
