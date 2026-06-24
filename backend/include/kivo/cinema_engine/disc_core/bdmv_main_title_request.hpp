#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct BdmvMainTitleRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::vector<std::string> playlist_files;
    std::vector<int64_t> playlist_durations;
    std::vector<int> playlist_chapters;
    std::vector<int64_t> playlist_sizes;
    std::string user_override_playlist;
};

} // namespace kivo::cinema_engine
