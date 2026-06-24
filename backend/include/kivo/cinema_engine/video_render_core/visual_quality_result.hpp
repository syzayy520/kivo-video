#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/video_render_core/visual_quality_metrics.hpp"

namespace kivo::cinema_engine {

struct VisualQualityResult {
    ContractMetadata metadata;
    VisualQualityMetrics metrics;
    bool meets_baseline{false};
    std::string quality_tier;  // "excellent", "good", "acceptable", "poor"
    std::vector<std::string> warnings;
};

}  // namespace kivo::cinema_engine