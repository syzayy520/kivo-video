// gpu_budget_evidence.hpp — GATE-024: P5A GPU budget evidence
#pragma once
#include <cstdint>
#include <string>
#include "gpu_budget_snapshot.hpp"

namespace kivo::video_plane::gpu {

struct GpuBudgetEvidence {
    GpuBudgetSnapshot snapshot;
    std::string evidence_id;
    std::string correlation_id;
    bool budget_exceeded{false};            // budget was exceeded
    bool operator==(GpuBudgetEvidence const&) const = default;
};

}  // namespace kivo::video_plane::gpu
