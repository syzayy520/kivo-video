#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Result of a range read operation with metadata.
struct RangeReadResult {
    ContractMetadata metadata;
    std::string request_id;
    bool success{false};
    int64_t bytes_read{0};
    int64_t response_time_ms{0};
    int64_t total_content_length{0};
    std::string etag;
    std::string error_message;
};

}  // namespace kivo::cinema_engine
