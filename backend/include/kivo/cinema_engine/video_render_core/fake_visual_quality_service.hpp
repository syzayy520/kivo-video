#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/video_render_core/visual_quality_result.hpp"

namespace kivo::cinema_engine {

VisualQualityResult build_excellent_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id);
VisualQualityResult build_good_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id);
VisualQualityResult build_acceptable_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id);
VisualQualityResult build_poor_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id);
VisualQualityResult build_custom_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id, const std::string& quality_tier, bool meets_baseline, const std::vector<std::string>& warnings);

}  // namespace kivo::cinema_engine