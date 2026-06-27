#pragma once

#include "kivo/video_plane/threat/threat_surface_id.hpp"
#include <string>
#include <vector>

namespace kivo::video_plane::threat {

struct P5ThreatMatrixGateMapping {
    // Maps each threat surface to its verification gate stage
    static ThreatSurfaceId surface_for_gate(std::string const& gate_name);
    static std::vector<std::string> gates_for_surface(ThreatSurfaceId surface);
    static bool is_surface_covered_at_gate(ThreatSurfaceId surface, std::string const& gate);
};

}  // namespace kivo::video_plane::threat
