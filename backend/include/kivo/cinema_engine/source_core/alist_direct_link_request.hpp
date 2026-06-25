#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Request for a direct download link from AList.
struct AListDirectLinkRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string file_path;
    std::string auth_token;
    int64_t expires_in_seconds{0};  // 0 = server default
};

}  // namespace kivo::cinema_engine
