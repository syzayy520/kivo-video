// mime_conflict_policy.hpp — GATE-021: P5A MIME conflict policy
#pragma once
#include <string>

namespace kivo::video_plane::attachment {

enum class MimeConflictResolution {
    Reject,                                 // reject conflicting attachment
    PreferContainer,                        // prefer container-declared MIME
    PreferSniffed,                          // prefer sniffed MIME
    UseSafest                               // use safest interpretation
};

struct MimeConflictPolicy {
    MimeConflictResolution resolution{MimeConflictResolution::UseSafest};
    std::string conflict_reason;            // description of conflict
    bool operator==(MimeConflictPolicy const&) const = default;
};

}  // namespace kivo::video_plane::attachment
