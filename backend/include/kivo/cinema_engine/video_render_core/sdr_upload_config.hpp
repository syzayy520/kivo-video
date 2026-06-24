#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct SdrUploadConfig {
    ContractMetadata metadata;
    std::string upload_id;
    bool enable_color_correction{true};
    bool enable_sharpening{false};
    float brightness_adjustment{0.0f};
    float contrast_adjustment{1.0f};
    float saturation_adjustment{1.0f};
};

}  // namespace kivo::cinema_engine