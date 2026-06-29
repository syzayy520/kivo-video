#include "video/subtitle_plane/privacy/privacy_redline_policy.hpp"

namespace kivo::video::subtitle_plane::runtime {

bool privacy_mode_is_export_safe(const KivoSubtitlePrivacyMode mode) noexcept {
    return mode == KivoSubtitlePrivacyMode_Strict ||
           mode == KivoSubtitlePrivacyMode_DiagnosticRedacted;
}

}  // namespace kivo::video::subtitle_plane::runtime
