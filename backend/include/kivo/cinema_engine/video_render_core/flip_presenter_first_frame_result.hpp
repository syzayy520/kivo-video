#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct FlipPresenterFirstFrameResult {
    ContractMetadata metadata;
    std::string config_id;
    bool success{false};
    bool first_frame_presented{false};
    int64_t presentation_time_us{0};
    std::string error_message;
    bool hdr_active{false};
    bool zero_copy_active{false};
};

}  // namespace kivo::cinema_engine