#include "kivo/cinema_engine/resource_core/fake_memory_budget_service.hpp"

namespace kivo::cinema_engine {

MemoryBudget build_default_memory_budget(const std::string& budget_id) {
    MemoryBudget budget;
    budget.metadata.schema_version = "1.1";
    budget.budget_id = budget_id;
    budget.total_budget_bytes = 256 * 1024 * 1024; // 256MB
    budget.video_queue_bytes = 0;
    budget.audio_queue_bytes = 0;
    budget.decode_buffer_bytes = 0;
    budget.cache_bytes = 0;
    budget.is_over_budget = false;
    budget.utilization_pct = 0.0;
    return budget;
}

MemoryBudget build_memory_budget(const std::string& budget_id, int64_t total_budget_bytes) {
    MemoryBudget budget;
    budget.metadata.schema_version = "1.1";
    budget.budget_id = budget_id;
    budget.total_budget_bytes = total_budget_bytes;
    budget.video_queue_bytes = 0;
    budget.audio_queue_bytes = 0;
    budget.decode_buffer_bytes = 0;
    budget.cache_bytes = 0;
    budget.is_over_budget = false;
    budget.utilization_pct = 0.0;
    return budget;
}

MemoryBudget build_over_budget_memory(const std::string& budget_id) {
    MemoryBudget budget;
    budget.metadata.schema_version = "1.1";
    budget.budget_id = budget_id;
    budget.total_budget_bytes = 100 * 1024 * 1024; // 100MB
    budget.video_queue_bytes = 80 * 1024 * 1024;   // 80MB
    budget.audio_queue_bytes = 20 * 1024 * 1024;   // 20MB
    budget.decode_buffer_bytes = 10 * 1024 * 1024; // 10MB
    budget.cache_bytes = 5 * 1024 * 1024;          // 5MB
    budget.is_over_budget = true;
    budget.utilization_pct = 115.0;
    return budget;
}

MemoryBudget build_memory_budget_with_usage(
    const std::string& budget_id,
    int64_t video_queue_bytes,
    int64_t audio_queue_bytes,
    int64_t decode_buffer_bytes,
    int64_t cache_bytes
) {
    MemoryBudget budget;
    budget.metadata.schema_version = "1.1";
    budget.budget_id = budget_id;
    budget.total_budget_bytes = 256 * 1024 * 1024; // 256MB
    budget.video_queue_bytes = video_queue_bytes;
    budget.audio_queue_bytes = audio_queue_bytes;
    budget.decode_buffer_bytes = decode_buffer_bytes;
    budget.cache_bytes = cache_bytes;
    int64_t total_used = video_queue_bytes + audio_queue_bytes + decode_buffer_bytes + cache_bytes;
    budget.is_over_budget = (total_used > budget.total_budget_bytes);
    budget.utilization_pct = (static_cast<double>(total_used) / budget.total_budget_bytes) * 100.0;
    return budget;
}

}  // namespace kivo::cinema_engine