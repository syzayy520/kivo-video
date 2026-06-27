// protected_boundary_evidence.hpp — GATE-011: P5A protected boundary evidence (NEW)
#pragma once
#include <cstdint>
#include <string>
#include "protected_boundary_hint.hpp"
#include "protected_playback_capability_state.hpp"

namespace kivo::video_plane::protected_content {

struct ProtectedBoundaryEvidence {
    ProtectedBoundaryHint boundary_hint;
    ProtectedPlaybackCapabilityState capability_state;
    std::string evidence_id;
    std::string correlation_id;
    uint64_t timestamp{0};
    bool fail_closed{false};                // true if pipeline must fail-closed
    bool operator==(ProtectedBoundaryEvidence const&) const = default;
};

}  // namespace kivo::video_plane::protected_content
