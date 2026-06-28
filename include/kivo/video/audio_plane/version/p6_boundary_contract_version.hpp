// p6_boundary_contract_version.hpp — P6A version: P6 boundary contract version (§version)
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::version {

// P6BoundaryContractVersion
// Semantic version of the P6 boundary contract. Compile-only.
struct P6BoundaryContractVersion {
    uint32_t major{2};
    uint32_t minor{0};
    uint32_t patch{0};
    bool operator==(const P6BoundaryContractVersion&) const = default;
};

}  // namespace kivo::video::audio_plane::version
