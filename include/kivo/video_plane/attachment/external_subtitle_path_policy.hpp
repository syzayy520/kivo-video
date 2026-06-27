// external_subtitle_path_policy.hpp — GATE-021: P5A external subtitle path policy
#pragma once
#include <string>

namespace kivo::video_plane::attachment {

enum class ExternalSubtitlePathAction {
    Allow,                                  // allow external subtitle path
    AllowSandboxed,                         // allow but sandbox access
    Reject,                                 // reject external subtitle path
    Quarantine                              // quarantine for inspection
};

struct ExternalSubtitlePathPolicy {
    ExternalSubtitlePathAction action{ExternalSubtitlePathAction::AllowSandboxed};
    std::string rejection_reason;           // reason if rejected
    bool require_absolute_path{true};       // require absolute path
    bool operator==(ExternalSubtitlePathPolicy const&) const = default;
};

}  // namespace kivo::video_plane::attachment
