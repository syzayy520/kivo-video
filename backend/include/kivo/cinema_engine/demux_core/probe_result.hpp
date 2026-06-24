#pragma once

#include <string>

#include "kivo/cinema_engine/demux_core/stream_inventory.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct ProbeResult {
    ContractMetadata metadata;
    StreamInventory inventory;
    bool success{false};
    std::string error_message;
};

}  // namespace kivo::cinema_engine