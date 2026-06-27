// gpu_budget_provider.hpp — GATE-024: P5A GPU budget provider
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu {

struct GpuBudgetProvider {
    std::string provider_name;              // budget provider name
    uint64_t total_video_memory{0};         // total VRAM in bytes
    uint64_t available_video_memory{0};     // available VRAM in bytes
    bool is_query_supported{false};         // budget query is supported
    bool operator==(GpuBudgetProvider const&) const = default;
};

}  // namespace kivo::video_plane::gpu
