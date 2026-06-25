#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Declares what operations and features a source adapter supports.
struct SourceAdapterCapability {
    ContractMetadata metadata;
    std::string adapter_id;
    bool supports_range_read{false};
    bool supports_seek{false};
    bool supports_list_directory{false};
    bool supports_byte_range_cache{false};
    bool supports_connection_reuse{false};
    int64_t max_range_chunk_size{0};
};

}  // namespace kivo::cinema_engine
