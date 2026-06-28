// audio_license_boundary.hpp — P6A security: license boundary (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// AudioLicenseBoundary
// Boundary between claim and licensed production. Compile-only skeleton.
struct AudioLicenseBoundary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool claim_boundary_cannot_be_deferred{true};
    bool licensed_production_may_be_deferred{true};

    bool operator==(const AudioLicenseBoundary&) const = default;
};

}  // namespace kivo::video::audio_plane::security
