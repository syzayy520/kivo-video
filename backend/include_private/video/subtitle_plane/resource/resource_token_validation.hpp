#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus validate_resource_access_token(
    KivoSubtitleResourceAccessToken token,
    KivoSubtitleResourceAccessKind expected_kind) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
