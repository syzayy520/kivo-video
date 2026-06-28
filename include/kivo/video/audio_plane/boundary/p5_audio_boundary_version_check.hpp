// p5_audio_boundary_version_check.hpp — P6A boundary: P5 boundary version check (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P5AudioBoundaryVersionCheck
// Compile-time / handshake descriptor for verifying that the P5↔P6 boundary
// contract revision expected by one side matches the revision offered by the
// other. `compatible` is a semantic flag only; no runtime negotiation here.
struct P5AudioBoundaryVersionCheck {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint32_t expected_version{2};
    uint32_t actual_version{0};
    bool compatible{false};

    bool operator==(const P5AudioBoundaryVersionCheck&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary
