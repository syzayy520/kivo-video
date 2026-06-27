// protected_export_denied.hpp — P5D D8: Protected export denied
#pragma once
#include <cstdint>
#include <string>
#include "../export/frame_export_security_policy.hpp"
#include "../protected_content/protected_boundary_hint.hpp"

namespace kivo::video_plane::decoder {

struct ExportDenialResult {
    bool denied{false};
    std::string reason;
    std::string evidence_id;
};

// Check if frame export should be denied for protected content
inline ExportDenialResult check_protected_export(
    protected_content::ProtectedBoundaryHint const& hint,
    export_policy::FrameExportSecurityPolicy const& policy) {
    ExportDenialResult result;
    if (hint.kind != protected_content::ProtectedBoundaryKind::None) {
        if (policy.strip_protected_content) {
            result.denied = true;
            result.reason = "protected_content_export_denied";
            result.evidence_id = "export_denial_protected";
            return result;
        }
    }
    if (!policy.allow_hdr_export && hint.requires_secure_decode) {
        result.denied = true;
        result.reason = "secure_decode_export_denied";
        result.evidence_id = "export_denial_secure_decode";
    }
    return result;
}

}  // namespace kivo::video_plane::decoder
