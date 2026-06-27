// patent_risk_report.hpp — GATE-027: P5A patent risk report
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::license {

enum class PatentRiskLevel {
    None,                                   // no known patent risk
    Low,                                    // low patent risk
    Medium,                                 // medium patent risk
    High,                                   // high patent risk
    Unknown                                 // patent risk unknown
};

struct PatentRiskReport {
    PatentRiskLevel level{PatentRiskLevel::Unknown};
    std::string patent_category;            // e.g. "H.264", "HEVC", "AV1"
    std::string risk_description;           // human-readable risk description
    std::string mitigation;                 // mitigation strategy
    bool operator==(PatentRiskReport const&) const = default;
};

}  // namespace kivo::video_plane::license
