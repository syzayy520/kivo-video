#pragma once

#include "kivo/cinema_engine/demux_core/stream_inventory.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

StreamInventory build_fake_stream_inventory(const ContractMetadata& metadata);

}  // namespace kivo::cinema_engine

