#pragma once

#include <cstdint>

namespace kivo::video_plane::evidence {

struct P5NonCriticalEvidenceDropCounter {
    uint64_t total_dropped{0};              // atomic counter, monotonic
    uint64_t dropped_since_last_report{0};  // resettable
    uint64_t last_report_time{0};

    // Contract:
    // - Increment on every NonCriticalDroppable evidence drop
    // - Reset dropped_since_last_report on report emission
    // - Emit EvidenceDropReport when dropped_since_last_report > threshold
    bool operator==(P5NonCriticalEvidenceDropCounter const&) const = default;
};

}  // namespace kivo::video_plane::evidence
