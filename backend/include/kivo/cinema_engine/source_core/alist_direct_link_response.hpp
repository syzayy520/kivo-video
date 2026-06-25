#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Response with a direct download link from AList.
struct AListDirectLinkResponse {
    ContractMetadata metadata;
    std::string request_id;
    std::string direct_link_url;
    int64_t ttl_seconds{0};
    std::string content_type;
    int64_t content_length{0};
};

}  // namespace kivo::cinema_engine
