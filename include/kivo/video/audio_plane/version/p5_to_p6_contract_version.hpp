// p5_to_p6_contract_version.hpp — P6A version: P5→P6 contract version (§version)
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::version {

// P5ToP6ContractVersion
// Semantic version of the P5→P6 contract. Compile-only.
struct P5ToP6ContractVersion {
    uint32_t major{2};
    uint32_t minor{0};
    uint32_t patch{0};
    bool operator==(const P5ToP6ContractVersion&) const = default;
};

}  // namespace kivo::video::audio_plane::version
