#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// HDR detection result from stream analysis.
// Captures input signal type and Dolby Vision metadata
// without making false DV claims.
struct HdrDetectionResult {
    ContractMetadata metadata;
    std::string input_signal;  // "SDR", "HDR10", "HLG", "DV_Profile5", etc.
    bool hdr_metadata_present{false};
    bool dv_rpu_present{false};
    std::string dv_profile;
    std::string detection_confidence;
};

}  // namespace kivo::cinema_engine
