// dwm_latency_hint.hpp -- P5F F6: DWM latency hint evidence
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_render {

struct DwmLatencyHint {
    int64_t target_latency_ms{0};
    int64_t actual_composition_latency_ms{0};
    bool hint_applied{false};
    std::string evidence_id;
};

class FakeDwmLatencyTracker {
public:
    DwmLatencyHint set_hint(int64_t target_ms) {
        DwmLatencyHint h;
        h.target_latency_ms = target_ms;
        h.hint_applied = true;
        h.evidence_id = "dwm_latency_hint_" + std::to_string(++hint_count_);
        last_hint_ = h;
        return h;
    }

    void record_composition_latency(int64_t actual_ms) {
        last_hint_.actual_composition_latency_ms = actual_ms;
    }

    DwmLatencyHint const& last_hint() const { return last_hint_; }
    uint32_t hint_count() const { return hint_count_; }

    bool is_latency_within_budget(int64_t budget_ms) const {
        return last_hint_.actual_composition_latency_ms <= budget_ms;
    }

private:
    DwmLatencyHint last_hint_;
    uint32_t hint_count_{0};
};

}  // namespace kivo::video_plane::gpu_render
