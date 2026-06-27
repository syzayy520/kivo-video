// protected_boundary_hint.hpp — GATE-011: P5A protected boundary hint
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::protected_content {

enum class ProtectedBoundaryKind {
    None,                                   // no protection required
    AACS,                                   // AACS protected content
    Cinavia,                                // Cinavia watermark
    HDCP,                                   // HDCP output protection
    HardwareDRM,                            // hardware DRM session
    SoftwareDRM                             // software DRM session
};

struct ProtectedBoundaryHint {
    ProtectedBoundaryKind kind{ProtectedBoundaryKind::None};
    std::string content_id;                 // opaque content identifier
    std::string policy_reference;           // policy reference id
    bool requires_secure_decode{false};     // requires secure decode session
    bool operator==(ProtectedBoundaryHint const&) const = default;
};

}  // namespace kivo::video_plane::protected_content
