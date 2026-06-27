// boundary_contract_version.hpp — GATE-007: P5A boundary contract version
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::version {

struct BoundaryContractVersion {
    uint32_t major{1};
    uint32_t minor{0};
    uint32_t patch{0};
    std::string contract_family;            // e.g. "P5A-VideoPlane-Contracts"
    bool operator==(BoundaryContractVersion const&) const = default;
};

}  // namespace kivo::video_plane::version
