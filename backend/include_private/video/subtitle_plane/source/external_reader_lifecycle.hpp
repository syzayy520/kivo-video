#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitlePlaneError probe_external_reader_once(
    const KivoSubtitleSourceDescriptor& source) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
