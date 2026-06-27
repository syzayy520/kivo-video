// compatibility_policy.hpp — GATE-007: P5A compatibility policy / result
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::version {

enum class CompatibilityDecision {
    FullyCompatible,
    BackwardCompatible,
    ForwardCompatible,
    Incompatible,
    Unknown
};

struct CompatibilityResult {
    CompatibilityDecision decision{CompatibilityDecision::Unknown};
    BoundaryContractVersion consumer_version;
    BoundaryContractVersion producer_version;
    std::string reason;
    bool operator==(CompatibilityResult const&) const = default;
};

}  // namespace kivo::video_plane::version
