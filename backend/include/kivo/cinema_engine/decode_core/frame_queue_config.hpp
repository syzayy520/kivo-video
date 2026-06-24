#pragma once

#include <cstddef>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct FrameQueueConfig {
    ContractMetadata metadata;
    std::string queue_id;
    size_t max_frames{10};
    size_t max_bytes{100 * 1024 * 1024}; // 100MB
    bool drop_on_overflow{false};
};

}  // namespace kivo::cinema_engine