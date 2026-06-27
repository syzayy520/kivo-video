#pragma once

namespace kivo::video_plane::evidence {

enum class EvidenceLossPolicy {
    CriticalRingRequired,   // must enter critical ring
    NonCriticalDroppable,   // may be dropped
    Sampled,                // sampled, not every instance
    DebugOnly,              // debug-only, not in release
    GoldenRequired,         // must be kept for golden comparison
    SecurityRequired        // must be preserved for security audit
};

}  // namespace kivo::video_plane::evidence
