#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Stable identity for an Emby media server session.
struct EmbySessionIdentity {
    ContractMetadata metadata;
    std::string session_id;
    std::string server_id;
    std::string item_id;
    std::string media_source_id;
    std::string user_id;
    std::string device_id;
};

}  // namespace kivo::cinema_engine
