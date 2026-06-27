#pragma once

#include "kivo/video_plane/threat/threat_surface_id.hpp"
#include <string>

namespace kivo::video_plane::threat {

struct P5ThreatMitigationMatrixItem {
    std::string threat_id;
    ThreatSurfaceId threat_surface{ThreatSurfaceId::MaliciousContainer};
    std::string attack_vector;
    std::string affected_component;
    std::string risk_level;   // Critical/High/Medium/Low
    std::string mitigation;
    bool sandbox_required{false};
    std::string allocation_policy;
    std::string fuzz_target;
    std::string regression_corpus;
    std::string expected_failure_mode;
    std::string evidence_kind;
    std::string gate_stage;   // P5A/P5B/.../P5H/P5S
    bool operator==(P5ThreatMitigationMatrixItem const&) const = default;
};

}  // namespace kivo::video_plane::threat
