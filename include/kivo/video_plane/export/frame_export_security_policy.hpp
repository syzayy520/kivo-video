// frame_export_security_policy.hpp — GATE-034: P5A frame export security policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::export_policy {

struct FrameExportSecurityPolicy {
    bool allow_sdr_export{true};            // allow SDR frame export
    bool allow_hdr_export{false};           // allow HDR frame export (restricted)
    bool require_user_consent{true};        // require explicit user consent
    bool strip_protected_content{true};     // never export protected content frames
    uint32_t max_export_resolution{0};      // max export resolution (0 = unlimited)
    std::string export_reason;              // reason for export
    bool operator==(FrameExportSecurityPolicy const&) const = default;
};

}  // namespace kivo::video_plane::export_policy
