// surface_to_display_mapper.hpp — GATE-012: P5A surface to display mapper
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::protected_output {

struct SurfaceToDisplayMapper {
    uint64_t surface_id{0};                 // opaque surface identifier
    uint64_t display_id{0};                 // target display identifier
    std::string display_name;               // human-readable display name
    bool is_primary{false};                 // mapped to primary display
    bool operator==(SurfaceToDisplayMapper const&) const = default;
};

}  // namespace kivo::video_plane::protected_output
