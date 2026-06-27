// output_protection_runtime.hpp -- P5H H11: OutputProtection lost fail-closed
#pragma once
#include <cstdint>
#include <string>
#include "../protected_output/protected_output_status_snapshot.hpp"

namespace kivo::video_plane::color_closure {

using protected_output::ProtectedOutputStatusSnapshot;
using protected_output::ProtectedOutputCoverageLevel;

enum class OutputProtectionAction {
    ContinuePlayback,
    HoldLastSafeFrame,
    StopProtectedPresent,
    FailClosed
};

struct OutputProtectionResult {
    OutputProtectionAction action{OutputProtectionAction::ContinuePlayback};
    bool fail_closed{false};
    std::string evidence_id;
    std::string reason;
};

class FakeOutputProtectionHandler {
public:
    OutputProtectionResult evaluate(ProtectedOutputStatusSnapshot const& snapshot,
                                     bool has_protected_content) {
        OutputProtectionResult r;
        if (!has_protected_content) {
            r.action = OutputProtectionAction::ContinuePlayback;
            r.reason = "no_protected_content";
            return r;
        }
        // Protected content requires active output protection
        bool is_active = snapshot.coverage.coverage != ProtectedOutputCoverageLevel::None;
        bool has_unprotected = snapshot.coverage.unprotected_display_count > 0;
        if (!is_active) {
            r.action = OutputProtectionAction::FailClosed;
            r.fail_closed = true;
            r.reason = "output_protection_lost_during_protected_playback";
            r.evidence_id = "output_protection_lost_fail_closed";
        } else if (has_unprotected) {
            r.action = OutputProtectionAction::StopProtectedPresent;
            r.fail_closed = true;
            r.reason = "output_protection_lost_on_any_active_output";
            r.evidence_id = "output_protection_partial_fail_closed";
        } else {
            r.action = OutputProtectionAction::ContinuePlayback;
            r.reason = "output_protection_active";
        }
        return r;
    }
};

}  // namespace kivo::video_plane::color_closure
