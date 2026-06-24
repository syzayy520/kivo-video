#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct BdmvProbeResult {
    ContractMetadata metadata;
    std::string probe_id;
    bool success{false};
    bool is_bdmv_folder{false};
    bool is_mounted_iso{false};
    std::string error_message;
    std::vector<std::string> playlist_files;
    std::vector<std::string> stream_files;
    int64_t total_size_bytes{0};
};

}  // namespace kivo::cinema_engine
