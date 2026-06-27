// reorder_policy.hpp — GATE-020: P5A reorder policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::timebase {

enum class ReorderMode {
    None,                                   // no reordering
    PresentationOrder,                      // reorder to presentation order
    DecodeOrder,                            // keep decode order
    Adaptive                                // adaptive reordering
};

struct ReorderPolicy {
    ReorderMode mode{ReorderMode::PresentationOrder};
    uint32_t max_reorder_depth{16};         // max reorder buffer depth
    bool drop_late_frames{true};            // drop frames that arrive too late
    bool operator==(ReorderPolicy const&) const = default;
};

}  // namespace kivo::video_plane::timebase
