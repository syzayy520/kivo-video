// audio_software_path_verification_status.hpp — P6A capability: path status (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// SoftwarePathVerificationStatus
// How far the software path was verified. Compile-only enum.
enum class SoftwarePathVerificationStatus {
    NotVerified = 0,
    EndpointSubmissionVerified = 1,
    FullPathVerified = 2
};

}  // namespace kivo::video::audio_plane::capability
