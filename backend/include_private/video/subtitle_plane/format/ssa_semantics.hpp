#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

constexpr uint32_t kP8StyleFlagExplicitLineBreak = 1u << 0u;
constexpr uint32_t kP8StyleFlagNormalizedFallback = 1u << 1u;
constexpr uint32_t kP8StyleFlagWebVttPosition = 1u << 2u;
constexpr uint32_t kP8StyleFlagWebVttVertical = 1u << 3u;

KivoP8ApiStatus classify_ssa_semantics(KivoSubtitleInputStringView text,
                                       KivoSubtitleStyleSemanticsReport* out_report) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
