#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// MemoryBudget tracks memory usage for frame queues and decode buffers.
struct MemoryBudget {
    ContractMetadata metadata;
    std::string budget_id;
    int64_t total_budget_bytes{256 * 1024 * 1024};  // 256MB default
    int64_t video_queue_bytes{0};
    int64_t audio_queue_bytes{0};
    int64_t decode_buffer_bytes{0};
    int64_t cache_bytes{0};
    bool is_over_budget{false};
    double utilization_pct{0.0};
};

}  // namespace kivo::cinema_engine
