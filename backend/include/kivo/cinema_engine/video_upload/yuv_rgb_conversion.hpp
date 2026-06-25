#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// YuvRgbConversion defines the pixel format conversion policy.
// Actual conversion is done by the adapter boundary.
enum class ConversionQuality {
    Fast,        // Fast bilinear conversion
    Accurate,    // Accurate cubic conversion
    Default      // Use adapter default
};

// YuvRgbConversion is the policy for YUV→RGB/NV12 pixel format conversion.
struct YuvRgbConversion {
    ContractMetadata metadata;

    ConversionQuality quality{ConversionQuality::Default};
    bool preserve_alpha{false};
    bool use_bt709{true};  // BT.709 for HD content
    bool use_bt601{false}; // BT.601 for SD content

    // Get conversion quality as string.
    std::string quality_string() const {
        switch (quality) {
            case ConversionQuality::Fast: return "fast";
            case ConversionQuality::Accurate: return "accurate";
            default: return "default";
        }
    }
};

}  // namespace kivo::cinema_engine