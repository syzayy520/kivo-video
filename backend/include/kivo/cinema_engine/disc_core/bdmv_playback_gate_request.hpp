#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct BdmvPlaybackGateRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string folder_path;
    bool is_mounted_iso{false};
    std::string selected_playlist;
    bool is_encrypted{false};
};

} // namespace kivo::cinema_engine
