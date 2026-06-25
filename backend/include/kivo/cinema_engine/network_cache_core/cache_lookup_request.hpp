#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct CacheLookupRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string cache_id;
    std::string remote_object_identity_ref;
    uint64_t byte_offset{0};
    uint64_t byte_length{0};
};

}  // namespace kivo::cinema_engine
