#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct BdmvPlaylistDuplicateResult {
    ContractMetadata metadata;
    std::string request_id;
    bool success{false};
    std::vector<std::string> unique_playlists;
    std::vector<std::string> collapsed_playlists;
    std::string collapse_reason;
};

} // namespace kivo::cinema_engine
