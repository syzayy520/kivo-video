#pragma once

#include <cstddef>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct FrameQueueStatus {
    ContractMetadata metadata;
    std::string queue_id;
    size_t current_frames{0};
    size_t current_bytes{0};
    bool is_full{false};
    bool is_empty{true};
};

}  // namespace kivo::cinema_engine