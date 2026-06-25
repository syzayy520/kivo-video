#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct CacheLookupResult {
    ContractMetadata metadata;
    std::string request_id;
    bool hit{false};
    uint64_t readable_bytes{0};
    std::string matched_segment_id;
    std::string miss_reason;
};

}  // namespace kivo::cinema_engine
