// p6_to_p8_contract_version.hpp — P6A version: P6→P8 contract version (§version)
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::version {

// P6ToP8ContractVersion
// Semantic version of the P6→P8 contract. Compile-only.
struct P6ToP8ContractVersion {
    uint32_t major{2};
    uint32_t minor{0};
    uint32_t patch{0};
    bool operator==(const P6ToP8ContractVersion&) const = default;
};

}  // namespace kivo::video::audio_plane::version
