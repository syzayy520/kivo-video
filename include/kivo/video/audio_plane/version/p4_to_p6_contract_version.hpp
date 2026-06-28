// p4_to_p6_contract_version.hpp — P6A version: P4→P6 contract version (§version)
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::version {

// P4ToP6ContractVersion
// Semantic version of the P4→P6 contract. Compile-only.
struct P4ToP6ContractVersion {
    uint32_t major{2};
    uint32_t minor{0};
    uint32_t patch{0};
    bool operator==(const P4ToP6ContractVersion&) const = default;
};

}  // namespace kivo::video::audio_plane::version
