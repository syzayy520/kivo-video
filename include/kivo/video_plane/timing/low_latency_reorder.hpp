// low_latency_reorder.hpp -- P5E E10: low-latency reorder budget
// if reorder_delay <= target_latency_budget: normal reorder
// else if reorder_delay <= max_latency: catch-up (skip/drop)
// else: skip to live edge or fail
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::timing {

enum class LowLatencyReorderAction {
    NormalReorder,
    CatchUp,
    SkipToLiveEdge,
    Fail
};

struct LowLatencyReorderConfig {
    int64_t target_latency_budget_ms{500};    // target latency budget
    int64_t max_latency_ms{2000};             // max allowed latency before skip
    int64_t skip_to_live_edge_threshold_ms{3000}; // threshold to skip to live edge
};

struct LowLatencyReorderResult {
    LowLatencyReorderAction action{LowLatencyReorderAction::NormalReorder};
    int64_t reorder_delay_ms{0};
    std::string reason;
};

inline LowLatencyReorderResult evaluate_low_latency_reorder(
    int64_t reorder_delay_ms,
    LowLatencyReorderConfig const& cfg) {
    LowLatencyReorderResult r;
    r.reorder_delay_ms = reorder_delay_ms;
    if (reorder_delay_ms <= cfg.target_latency_budget_ms) {
        r.action = LowLatencyReorderAction::NormalReorder;
        r.reason = "within_target_budget";
    } else if (reorder_delay_ms <= cfg.max_latency_ms) {
        r.action = LowLatencyReorderAction::CatchUp;
        r.reason = "exceeds_target_within_max";
    } else if (reorder_delay_ms <= cfg.skip_to_live_edge_threshold_ms) {
        r.action = LowLatencyReorderAction::SkipToLiveEdge;
        r.reason = "exceeds_max_latency";
    } else {
        r.action = LowLatencyReorderAction::Fail;
        r.reason = "exceeds_skip_threshold";
    }
    return r;
}

}  // namespace kivo::video_plane::timing
