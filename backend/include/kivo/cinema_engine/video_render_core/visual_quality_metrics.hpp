#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct VisualQualityMetrics {
    ContractMetadata metadata;
    std::string metrics_id;
    float psnr{0.0f};  // Peak Signal-to-Noise Ratio
    float ssim{0.0f};  // Structural Similarity Index
    float vmaf{0.0f};  // Video Multimethod Assessment Fusion
    float bitrate_efficiency{0.0f};
    int32_t frame_count{0};
    bool is_hdr{false};
};

}  // namespace kivo::cinema_engine