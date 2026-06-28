// asio_boundary.hpp — P6A advanced_format: ASIO boundary (§advanced_format)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/advanced_format/dsd_format_boundary.hpp"
namespace kivo::video::audio_plane::advanced_format {

// AsioBoundary
// Boundary of ASIO support. Compile-only skeleton.
struct AsioBoundary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    DsdSupport support{DsdSupport::NotSupported};

    bool operator==(const AsioBoundary&) const = default;
};

}  // namespace kivo::video::audio_plane::advanced_format
