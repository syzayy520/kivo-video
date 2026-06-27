// active_output_set.hpp — GATE-012: P5A active output set
#pragma once
#include <cstdint>
#include <vector>
#include "surface_to_display_mapper.hpp"

namespace kivo::video_plane::protected_output {

struct ActiveOutputSet {
    std::vector<SurfaceToDisplayMapper> mappings;
    uint64_t primary_display_id{0};
    uint32_t active_display_count{0};
    bool operator==(ActiveOutputSet const&) const = default;
};

}  // namespace kivo::video_plane::protected_output
