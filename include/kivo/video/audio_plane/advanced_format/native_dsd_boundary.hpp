// native_dsd_boundary.hpp — P6A advanced_format: native DSD (§advanced_format)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/advanced_format/dsd_format_boundary.hpp"
namespace kivo::video::audio_plane::advanced_format {

// NativeDsdBoundary
// Boundary of native DSD support. Compile-only skeleton.
struct NativeDsdBoundary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    DsdSupport support{DsdSupport::NotSupported};

    bool operator==(const NativeDsdBoundary&) const = default;
};

}  // namespace kivo::video::audio_plane::advanced_format
