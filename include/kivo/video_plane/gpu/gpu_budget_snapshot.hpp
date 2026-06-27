// gpu_budget_snapshot.hpp — GATE-024: P5A GPU budget snapshot
#pragma once
#include <cstdint>
#include <string>
#include "budget_pressure_level.hpp"

namespace kivo::video_plane::gpu {

struct GpuBudgetSnapshot {
    uint64_t total_memory{0};               // total VRAM
    uint64_t used_memory{0};                // used VRAM
    uint64_t available_memory{0};           // available VRAM
    BudgetPressureLevel pressure{BudgetPressureLevel::Relaxed};
    uint64_t snapshot_timestamp{0};
    bool operator==(GpuBudgetSnapshot const&) const = default;
};

}  // namespace kivo::video_plane::gpu
