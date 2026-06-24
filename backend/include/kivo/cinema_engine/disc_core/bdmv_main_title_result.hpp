#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct BdmvMainTitleResult {
    ContractMetadata metadata;
    std::string request_id;
    bool success{false};
    std::string selected_playlist;
    std::string selection_reason;
    std::vector<std::string> all_playlists;
};

} // namespace kivo::cinema_engine
