// compliance_package.hpp — GATE-027: P5A compliance package
#pragma once
#include <string>
#include "patent_risk_report.hpp"
#include "redistribution_checklist.hpp"

namespace kivo::video_plane::license {

struct CompliancePackage {
    PatentRiskReport patent_report;
    RedistributionChecklist redistribution;
    std::string package_version;
    bool is_complete{false};                // compliance package is complete
    bool operator==(CompliancePackage const&) const = default;
};

}  // namespace kivo::video_plane::license
