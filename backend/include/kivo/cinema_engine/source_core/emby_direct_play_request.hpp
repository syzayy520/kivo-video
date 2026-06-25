#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Request for a direct play URL from Emby.
struct EmbyDirectPlayRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string item_id;
    std::string media_source_id;
    std::string device_profile_id;
    int64_t max_streaming_bitrate{0};  // 0 = unlimited
};

}  // namespace kivo::cinema_engine
