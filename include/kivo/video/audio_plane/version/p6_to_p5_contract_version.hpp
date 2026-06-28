// p6_to_p5_contract_version.hpp — P6A version: P6→P5 contract version (§version)
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::version {

// P6ToP5ContractVersion
// Semantic version of the P6→P5 contract. Compile-only.
struct P6ToP5ContractVersion {
    uint32_t major{2};
    uint32_t minor{0};
    uint32_t patch{0};
    bool operator==(const P6ToP5ContractVersion&) const = default;
};

}  // namespace kivo::video::audio_plane::version
