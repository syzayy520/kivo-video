// redistribution_checklist.hpp — GATE-027: P5A redistribution checklist
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::license {

struct RedistributionChecklist {
    std::vector<std::string> required_licenses; // licenses required for redistribution
    std::vector<std::string> required_notices;  // notices that must be included
    bool is_compliant{false};               // redistribution is compliant
    std::string non_compliance_reason;      // reason if not compliant
    bool operator==(RedistributionChecklist const&) const = default;
};

}  // namespace kivo::video_plane::license
