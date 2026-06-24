#pragma once

#include <string>

#include "kivo/cinema_engine/demux_core/stream_inventory.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct DemuxRequest {
    ContractMetadata metadata;
    std::string source_uri;
    StreamInventory inventory;
    bool fast_seek{true};
};

}  // namespace kivo::cinema_engine