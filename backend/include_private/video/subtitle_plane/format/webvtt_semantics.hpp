#pragma once

#include "video/subtitle_plane/format/ssa_semantics.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus classify_webvtt_semantics(KivoSubtitleInputStringView text,
                                          KivoSubtitleStyleSemanticsReport* out_report) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
