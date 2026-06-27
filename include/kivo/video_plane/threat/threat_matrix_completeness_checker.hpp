#pragma once

#include "kivo/video_plane/threat/threat_surface_id.hpp"
#include "kivo/video_plane/threat/threat_mitigation_matrix.hpp"
#include <vector>
#include <string>

namespace kivo::video_plane::threat {

struct P5ThreatMatrixCompletenessChecker {
    struct CompletenessResult {
        bool all_surfaces_covered{false};
        std::vector<ThreatSurfaceId> missing_surfaces;
        std::vector<std::string> items_without_test;
        std::vector<std::string> items_without_fuzz_target;
        std::string evidence_id;
    };

    static CompletenessResult check(P5ThreatMitigationMatrix const& matrix);
};

}  // namespace kivo::video_plane::threat
