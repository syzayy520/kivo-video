#pragma once

#include "kivo/cinema_engine/color_science_core/hdr_detection_result.hpp"
#include "kivo/cinema_engine/color_science_core/color_space_descriptor.hpp"

namespace kivo::cinema_engine {

// Validation result for HDR/DV claims.
struct HdrClaimValidation {
    ContractMetadata metadata;
    std::string validation_id;
    std::string claim_type;
    bool is_valid{false};
    std::string evidence_source;
    std::string failure_reason;
};

// Fake HDR/color service for P2-040 testing.
class FakeHdrColorService {
public:
    // Detection builders
    static HdrDetectionResult build_hdr10_detection();
    static HdrDetectionResult build_hlg_detection();
    static HdrDetectionResult build_dolby_vision_detection();
    static HdrDetectionResult build_sdr_detection();

    // Color space builders
    static ColorSpaceDescriptor build_sdr_color_space();
    static ColorSpaceDescriptor build_hdr10_color_space();

    // Claim validation builders
    static HdrClaimValidation build_valid_dolby_vision_claim();
    static HdrClaimValidation build_false_dolby_vision_claim();
};

}  // namespace kivo::cinema_engine
