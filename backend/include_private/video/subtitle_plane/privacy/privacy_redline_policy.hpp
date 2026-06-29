#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

bool privacy_mode_is_export_safe(KivoSubtitlePrivacyMode mode) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
