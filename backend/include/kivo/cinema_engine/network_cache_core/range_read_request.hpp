#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// A single range read request with byte offset and length.
struct RangeReadRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string reader_id;
    int64_t byte_offset{0};
    int64_t byte_length{0};
};

}  // namespace kivo::cinema_engine
