// gpu_budget_runtime.hpp -- P5G G13: GPU budget provider runtime
#pragma once
#include <cstdint>
#include <string>
#include "../gpu/budget_pressure_level.hpp"
#include "../gpu/gpu_budget_snapshot.hpp"

namespace kivo::video_plane::gpu_hw {

using gpu::BudgetPressureLevel;
using gpu::GpuBudgetSnapshot;

struct GpuBudgetResult {
    GpuBudgetSnapshot snapshot;
    BudgetPressureLevel pressure{BudgetPressureLevel::Relaxed};
    bool is_within_budget{true};
    std::string evidence_id;
};

class FakeGpuBudgetProvider {
public:
    void set_budget(uint64_t total_memory, uint64_t max_decode_surfaces,
                    uint64_t max_render_targets, uint64_t max_backbuffers) {
        total_memory_ = total_memory;
        max_decode_surfaces_ = max_decode_surfaces;
        max_render_targets_ = max_render_targets;
        max_backbuffers_ = max_backbuffers;
    }

    GpuBudgetResult check(uint64_t used_memory, uint64_t decode_surfaces,
                          uint64_t render_targets, uint64_t backbuffers) {
        GpuBudgetResult r;
        r.snapshot.total_memory = total_memory_;
        r.snapshot.used_memory = used_memory;
        r.snapshot.available_memory = total_memory_ > used_memory ? total_memory_ - used_memory : 0;

        double mem_pct = total_memory_ > 0 ?
            static_cast<double>(used_memory) / static_cast<double>(total_memory_) : 1.0;

        if (mem_pct > 0.9 || decode_surfaces > max_decode_surfaces_ ||
            render_targets > max_render_targets_ || backbuffers > max_backbuffers_) {
            r.pressure = BudgetPressureLevel::Critical;
            r.snapshot.pressure = BudgetPressureLevel::Critical;
            r.is_within_budget = false;
            r.evidence_id = "gpu_budget_critical_exceeded";
        } else if (mem_pct > 0.75) {
            r.pressure = BudgetPressureLevel::High;
            r.snapshot.pressure = BudgetPressureLevel::High;
            r.is_within_budget = true;
            r.evidence_id = "gpu_budget_high_pressure";
        } else if (mem_pct > 0.5) {
            r.pressure = BudgetPressureLevel::Moderate;
            r.snapshot.pressure = BudgetPressureLevel::Moderate;
            r.is_within_budget = true;
            r.evidence_id = "gpu_budget_moderate";
        } else {
            r.pressure = BudgetPressureLevel::Relaxed;
            r.snapshot.pressure = BudgetPressureLevel::Relaxed;
            r.is_within_budget = true;
            r.evidence_id = "gpu_budget_relaxed";
        }
        return r;
    }

private:
    uint64_t total_memory_{0};
    uint64_t max_decode_surfaces_{0};
    uint64_t max_render_targets_{0};
    uint64_t max_backbuffers_{0};
};

}  // namespace kivo::video_plane::gpu_hw
