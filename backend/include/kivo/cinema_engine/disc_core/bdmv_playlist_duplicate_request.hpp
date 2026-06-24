#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct BdmvPlaylistDuplicateRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::vector<std::string> playlist_files;
    std::vector<std::vector<std::string>> playlist_streams;
};

} // namespace kivo::cinema_engine
