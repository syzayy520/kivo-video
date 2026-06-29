#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitlePlaneError validate_source_descriptor(
    const KivoSubtitleSourceDescriptor& source) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
