#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitlePlaneRuntimeCaps default_runtime_caps() noexcept;
KivoP8ApiStatus merge_runtime_caps(const KivoSubtitlePlaneConfig* config,
                                   KivoSubtitlePlaneRuntimeCaps* out_caps) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
