#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Response from a WebDAV range request.
struct WebDavRangeResponse {
    ContractMetadata metadata;
    std::string request_id;
    int status_code{0};            // HTTP status (200, 206, 416, etc.)
    int64_t content_range_total{0};  // Total resource size from Content-Range
    int64_t content_range_start{0};  // Start byte from Content-Range
    int64_t content_range_end{0};    // End byte from Content-Range
    int64_t actual_byte_count{0};    // Actual bytes received in body
    bool is_full_content{false};     // True if 200 (full body, not ranged)
};

}  // namespace kivo::cinema_engine
