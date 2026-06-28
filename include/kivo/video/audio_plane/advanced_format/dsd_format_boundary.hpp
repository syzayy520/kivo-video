// dsd_format_boundary.hpp — P6A advanced_format: DSD boundary (§advanced_format)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::advanced_format {

// DsdSupport
// Level of DSD support. Compile-only enum.
enum class DsdSupport {
    NotSupported = 0,
    Deferred = 1,
    Future = 2
};

// DsdFormatBoundary
// Boundary of DSD format support. Compile-only skeleton.
struct DsdFormatBoundary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    DsdSupport support{DsdSupport::NotSupported};

    bool operator==(const DsdFormatBoundary&) const = default;
};

}  // namespace kivo::video::audio_plane::advanced_format
