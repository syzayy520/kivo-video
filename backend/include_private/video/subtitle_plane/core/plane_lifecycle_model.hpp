#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitleTimelineAvailability timeline_for_plane_state(
    KivoSubtitlePlaneState state) noexcept;
bool plane_state_allows_frame_build(KivoSubtitlePlaneState state) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
