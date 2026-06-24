#pragma once

#include "kivo/cinema_engine/resource_core/memory_budget.hpp"

namespace kivo::cinema_engine {

MemoryBudget build_default_memory_budget(const std::string& budget_id);
MemoryBudget build_memory_budget(const std::string& budget_id, int64_t total_budget_bytes);
MemoryBudget build_over_budget_memory(const std::string& budget_id);
MemoryBudget build_memory_budget_with_usage(
    const std::string& budget_id,
    int64_t video_queue_bytes,
    int64_t audio_queue_bytes,
    int64_t decode_buffer_bytes,
    int64_t cache_bytes
);

}  // namespace kivo::cinema_engine