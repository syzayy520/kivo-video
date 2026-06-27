// p010_formula.hpp — GATE-015: P5A P010 formula
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::p010 {

enum class P010AccessMode {
    IntegerPlane,                           // access as integer plane (preferred)
    UNorm,                                  // access as unorm (fallback)
    Mixed                                   // mixed access mode
};

struct P010Formula {
    P010AccessMode access_mode{P010AccessMode::IntegerPlane};
    uint32_t bits_per_component{10};        // 10-bit components
    bool is_little_endian{true};            // P010 is little-endian
    std::string formula_reference;          // reference to formula documentation
    bool operator==(P010Formula const&) const = default;
};

}  // namespace kivo::video_plane::p010
