// audio_drm_boundary.hpp — P6A format: audio drm boundary contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
enum class AudioDrmBoundary {
    Unprotected = 0,
    SoftwareAllowed = 1,
    PassthroughAllowed = 2,
    BoundaryOnly = 3,
    FailClosed = 4
};
struct AudioDrmBoundaryContract {
    kivo::video::audio_plane::ContractMetadata metadata;
    AudioDrmBoundary boundary{AudioDrmBoundary::FailClosed};
};
}  // namespace kivo::video::audio_plane::format
