// p010_unorm_tolerance.hpp — GATE-015: P5A P010 unorm tolerance
#pragma once
#include <cstdint>

namespace kivo::video_plane::p010 {

struct P010UnormTolerance {
    uint32_t lsb_tolerance{0};              // tolerance in LSBs
    uint32_t max_drift{0};                  // max drift from integer path
    bool allow_rounding{true};              // allow rounding during unorm access
    bool operator==(P010UnormTolerance const&) const = default;
};

}  // namespace kivo::video_plane::p010
