#include "kivo/cinema_engine/video_render_core/fake_visual_quality_service.hpp"

namespace kivo::cinema_engine {

VisualQualityResult build_excellent_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id) {
    VisualQualityResult result;
    result.metadata = metadata;
    result.metrics.metadata = metadata;
    result.metrics.metrics_id = metrics_id;
    result.metrics.psnr = 45.0f;
    result.metrics.ssim = 0.98f;
    result.metrics.vmaf = 95.0f;
    result.metrics.bitrate_efficiency = 0.95f;
    result.metrics.frame_count = 1000;
    result.metrics.is_hdr = false;
    result.meets_baseline = true;
    result.quality_tier = "excellent";
    return result;
}

VisualQualityResult build_good_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id) {
    VisualQualityResult result;
    result.metadata = metadata;
    result.metrics.metadata = metadata;
    result.metrics.metrics_id = metrics_id;
    result.metrics.psnr = 38.0f;
    result.metrics.ssim = 0.92f;
    result.metrics.vmaf = 85.0f;
    result.metrics.bitrate_efficiency = 0.85f;
    result.metrics.frame_count = 1000;
    result.metrics.is_hdr = false;
    result.meets_baseline = true;
    result.quality_tier = "good";
    return result;
}

VisualQualityResult build_acceptable_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id) {
    VisualQualityResult result;
    result.metadata = metadata;
    result.metrics.metadata = metadata;
    result.metrics.metrics_id = metrics_id;
    result.metrics.psnr = 30.0f;
    result.metrics.ssim = 0.85f;
    result.metrics.vmaf = 70.0f;
    result.metrics.bitrate_efficiency = 0.70f;
    result.metrics.frame_count = 1000;
    result.metrics.is_hdr = false;
    result.meets_baseline = true;
    result.quality_tier = "acceptable";
    return result;
}

VisualQualityResult build_poor_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id) {
    VisualQualityResult result;
    result.metadata = metadata;
    result.metrics.metadata = metadata;
    result.metrics.metrics_id = metrics_id;
    result.metrics.psnr = 20.0f;
    result.metrics.ssim = 0.70f;
    result.metrics.vmaf = 50.0f;
    result.metrics.bitrate_efficiency = 0.50f;
    result.metrics.frame_count = 1000;
    result.metrics.is_hdr = false;
    result.meets_baseline = false;
    result.quality_tier = "poor";
    result.warnings.push_back("Visual quality below baseline threshold");
    return result;
}

VisualQualityResult build_custom_visual_quality_result(const ContractMetadata& metadata, const std::string& metrics_id, const std::string& quality_tier, bool meets_baseline, const std::vector<std::string>& warnings) {
    VisualQualityResult result;
    result.metadata = metadata;
    result.metrics.metadata = metadata;
    result.metrics.metrics_id = metrics_id;
    result.metrics.psnr = 35.0f;
    result.metrics.ssim = 0.90f;
    result.metrics.vmaf = 80.0f;
    result.metrics.bitrate_efficiency = 0.80f;
    result.metrics.frame_count = 1000;
    result.metrics.is_hdr = false;
    result.meets_baseline = meets_baseline;
    result.quality_tier = quality_tier;
    result.warnings = warnings;
    return result;
}

}  // namespace kivo::cinema_engine