#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct BdmvProbeRequest {
    ContractMetadata metadata;
    std::string probe_id;
    std::string folder_path;
    bool is_mounted_iso{false};
};

}  // namespace kivo::cinema_engine
