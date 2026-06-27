#pragma once

#include "kivo/video_plane/threat/threat_mitigation_matrix_item.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace kivo::video_plane::threat {

struct P5ThreatMitigationMatrix {
    std::vector<P5ThreatMitigationMatrixItem> items;
    std::string matrix_version;
    uint64_t last_updated{0};

    // Contract:
    // - Must contain all 29 threat surfaces
    // - Each threat must have >= 1 mitigation
    // - Each mitigation must have >= 1 test name
    // - Must not have only quarantine as mitigation
    // - Matrix completeness checked by GATE-003 checker
    bool operator==(P5ThreatMitigationMatrix const&) const = default;
};

}  // namespace kivo::video_plane::threat
