// dop_transport_boundary.hpp — P6A advanced_format: DoP transport (§advanced_format)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/advanced_format/dsd_format_boundary.hpp"
namespace kivo::video::audio_plane::advanced_format {

// DopTransportBoundary
// Boundary of DoP (DSD over PCM) transport. Compile-only skeleton.
struct DopTransportBoundary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    DsdSupport support{DsdSupport::NotSupported};

    bool operator==(const DopTransportBoundary&) const = default;
};

}  // namespace kivo::video::audio_plane::advanced_format
