#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Request to read a byte range from a WebDAV URL.
struct WebDavRangeRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string url;
    int64_t byte_offset{0};
    int64_t byte_length{0};
    std::string etag;          // Optional: cached ETag for conditional request
    std::string if_range;      // Optional: If-Range header value
};

}  // namespace kivo::cinema_engine
