#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus compute_redacted_identity(
    const KivoSubtitlePrivacyPolicy* privacy,
    const KivoSubtitleSourceDescriptor& source,
    KivoRedactedSourceIdentity* out_identity) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
