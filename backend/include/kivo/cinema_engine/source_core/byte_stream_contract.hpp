#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// ByteStream provides sequential read access to a byte source.
// This is the thinnest read abstraction for local file and network sources.
struct ByteStreamContract {
    ContractMetadata metadata;
    std::string stream_id;
    std::string source_kind;  // "local_file", "network_range", "cache_segment"
    int64_t position{0};
    int64_t length{-1};       // -1 if unknown
    bool is_seekable{false};
    bool is_readable{false};
};

}  // namespace kivo::cinema_engine
