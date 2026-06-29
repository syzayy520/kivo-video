#pragma once

namespace kivo::video::playback_graph::runtime {

struct ProtectedContentInputs {
    bool protected_content{false};
    bool secure_output_available{true};
    bool stale_frame_visible{false};
};

struct ProtectedContentDecision {
    bool playback_allowed{true};
    bool evidence_redacted{false};
    bool screenshots_disabled{false};
    bool thumbnails_disabled{false};
    bool stale_frame_blocked{false};
    bool resource_retire_blocking{false};
};

class ProtectedContentPolicy {
public:
    [[nodiscard]] ProtectedContentDecision decide(const ProtectedContentInputs& inputs) noexcept;
};

}  // namespace kivo::video::playback_graph::runtime
