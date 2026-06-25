#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Identity and capability declaration for a network range reader.
// Sits behind the source adapter boundary.
struct NetworkRangeReader {
    ContractMetadata metadata;
    std::string reader_id;
    std::string adapter_id;
    std::string base_url;
    bool supports_range{true};
    int64_t max_range_size{0};  // 0 = no limit
};

}  // namespace kivo::cinema_engine
