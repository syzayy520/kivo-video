#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus poll_cancelled(const KivoSubtitleCancellationView& view,
                               uint32_t* out_cancelled) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
