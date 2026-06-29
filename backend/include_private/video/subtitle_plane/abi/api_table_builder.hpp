#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

bool host_is_little_endian() noexcept;
KivoSubtitlePlaneApiV1 make_api_table() noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
