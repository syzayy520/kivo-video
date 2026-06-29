#include "video/playback_graph/coordination/protected_content_policy.hpp"

namespace kivo::video::playback_graph::runtime {

ProtectedContentDecision ProtectedContentPolicy::decide(
    const ProtectedContentInputs& inputs) noexcept {
    ProtectedContentDecision decision{};
    if (!inputs.protected_content) {
        return decision;
    }

    decision.playback_allowed = inputs.secure_output_available;
    decision.evidence_redacted = true;
    decision.screenshots_disabled = true;
    decision.thumbnails_disabled = true;
    decision.stale_frame_blocked = inputs.stale_frame_visible;
    decision.resource_retire_blocking = true;
    return decision;
}

}  // namespace kivo::video::playback_graph::runtime
