// protected_boundary_runtime.hpp -- P5H H10: Protected BoundaryOnly closure
// No platform backend -> deny decode/present, output ProtectedBoundaryOnlyFailClosedEvidence
#pragma once
#include <cstdint>
#include <string>
#include "../protected_content/protected_boundary_hint.hpp"
#include "../protected_content/protected_playback_capability_state.hpp"

namespace kivo::video_plane::color_closure {

using protected_content::ProtectedBoundaryHint;
using protected_content::ProtectedBoundaryKind;
using protected_content::ProtectedPlaybackCapabilityState;
using protected_content::ProtectedPlaybackState;

struct ProtectedBoundaryResult {
    bool decode_allowed{false};
    bool present_allowed{false};
    bool fail_closed{false};
    std::string evidence_id;
    std::string reason;
    ProtectedPlaybackState final_state{ProtectedPlaybackState::NotRequired};
};

class FakeProtectedBoundaryClosure {
public:
    // BoundaryOnly: no platform backend, deny everything
    ProtectedBoundaryResult evaluate_boundary_only(ProtectedBoundaryHint const& hint) {
        ProtectedBoundaryResult r;
        if (hint.kind == ProtectedBoundaryKind::None) {
            // No protection required -> allow
            r.decode_allowed = true;
            r.present_allowed = true;
            r.final_state = ProtectedPlaybackState::NotRequired;
            r.evidence_id = "protected_boundary_not_required";
            r.reason = "no_protection_required";
        } else {
            // Protection required but BoundaryOnly (no platform backend)
            r.decode_allowed = false;
            r.present_allowed = false;
            r.fail_closed = true;
            r.final_state = ProtectedPlaybackState::Failed;
            r.evidence_id = "ProtectedBoundaryOnlyFailClosedEvidence";
            r.reason = "boundary_only_no_platform_backend_fail_closed";
        }
        return r;
    }

    // With platform backend available
    ProtectedBoundaryResult evaluate_with_backend(
        ProtectedBoundaryHint const& hint,
        bool secure_decode_available,
        bool protected_output_available) {
        ProtectedBoundaryResult r;
        if (hint.kind == ProtectedBoundaryKind::None) {
            r.decode_allowed = true;
            r.present_allowed = true;
            r.final_state = ProtectedPlaybackState::NotRequired;
            r.evidence_id = "protected_not_required";
            return r;
        }
        if (secure_decode_available && protected_output_available) {
            r.decode_allowed = true;
            r.present_allowed = true;
            r.final_state = ProtectedPlaybackState::Established;
            r.evidence_id = "protected_established";
            r.reason = "secure_decode_and_protected_output_available";
        } else {
            r.decode_allowed = false;
            r.present_allowed = false;
            r.fail_closed = true;
            r.final_state = ProtectedPlaybackState::Failed;
            r.evidence_id = "protected_fail_closed";
            r.reason = "missing_secure_decode_or_protected_output";
        }
        return r;
    }
};

}  // namespace kivo::video_plane::color_closure
