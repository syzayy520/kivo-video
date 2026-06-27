// live_edge_hint_priority.hpp — GATE-009: P5A live edge hint priority
#pragma once

namespace kivo::video_plane::recovery {

enum class LiveEdgeHintPriority {
    P4Approved,                             // P4 explicitly approved
    P4TimelineOriginHint,                   // P4 timeline origin
    RecoveryBookmark,                       // last P5 recovery bookmark
    P5Estimate,                             // P5 estimation
    ConservativeLiveRebase                  // fallback conservative
};

}  // namespace kivo::video_plane::recovery
