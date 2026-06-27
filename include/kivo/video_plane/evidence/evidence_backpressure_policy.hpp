#pragma once

#include <cstdint>

namespace kivo::video_plane::evidence {

struct P5EvidenceBackpressurePolicy {
    bool release_never_blocks{true};  // Release build: never block on evidence sink
    bool debug_may_fail_fast{true};   // Debug build: may fail-fast if sink blocked
    uint64_t max_block_duration_ms{0};  // Max block duration (0 = never block)

    // Contract:
    // - Release: if sink unavailable, drop non-critical, ring critical
    // - Debug: may fail-fast if sink blocked > threshold
    // - Never block render hot path
    // - Never block during COM release
    bool operator==(P5EvidenceBackpressurePolicy const&) const = default;
};

}  // namespace kivo::video_plane::evidence
