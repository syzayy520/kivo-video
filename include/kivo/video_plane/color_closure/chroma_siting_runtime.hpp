// chroma_siting_runtime.hpp -- P5H H3: chroma siting golden tests
// MPEG2 Left / JPEG Center / TopLeft / Unknown conservative default
#pragma once
#include <cstdint>
#include <string>
#include "../color/chroma_siting_plan.hpp"

namespace kivo::video_plane::color_closure {

using color::ChromaSiting;
using color::ChromaSitingPlan;

struct ChromaSitingGoldenResult {
    ChromaSitingPlan plan;
    double horizontal_offset{0.0};
    double vertical_offset{0.0};
    bool is_golden_match{false};
    std::string source_standard;
};

// Get chroma siting offsets for known standards
inline ChromaSitingGoldenResult evaluate_chroma_siting(const std::string& standard) {
    ChromaSitingGoldenResult r;
    r.source_standard = standard;
    if (standard == "MPEG2") {
        r.plan.horizontal = ChromaSiting::Left;
        r.plan.vertical = ChromaSiting::Center;
        r.horizontal_offset = 0.0;
        r.vertical_offset = 0.5;
        r.is_golden_match = true;
    } else if (standard == "JPEG") {
        r.plan.horizontal = ChromaSiting::Center;
        r.plan.vertical = ChromaSiting::Center;
        r.horizontal_offset = 0.5;
        r.vertical_offset = 0.5;
        r.is_golden_match = true;
    } else if (standard == "TopLeft") {
        r.plan.horizontal = ChromaSiting::Left;
        r.plan.vertical = ChromaSiting::TopLeft;
        r.horizontal_offset = 0.0;
        r.vertical_offset = 0.0;
        r.is_golden_match = true;
    } else {
        // Unknown -> conservative default (Left/Center)
        r.plan.horizontal = ChromaSiting::Left;
        r.plan.vertical = ChromaSiting::Center;
        r.horizontal_offset = 0.0;
        r.vertical_offset = 0.5;
        r.is_golden_match = false;
        r.source_standard = "Unknown_conservative_default";
    }
    r.plan.source = r.source_standard;
    return r;
}

}  // namespace kivo::video_plane::color_closure
