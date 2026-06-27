// protected_output_coverage.hpp — GATE-012: P5A protected output coverage
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::protected_output {

enum class ProtectedOutputCoverageLevel {
    None,                                   // no protected output coverage
    Partial,                                // some outputs are protected
    Full,                                   // all active outputs are protected
    Unknown                                 // coverage could not be determined
};

struct ProtectedOutputCoverage {
    ProtectedOutputCoverageLevel coverage{ProtectedOutputCoverageLevel::None};
    uint32_t protected_display_count{0};
    uint32_t unprotected_display_count{0};
    std::string hdcp_version;               // HDCP version if applicable
    bool operator==(ProtectedOutputCoverage const&) const = default;
};

}  // namespace kivo::video_plane::protected_output
