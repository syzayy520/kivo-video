#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

bool subtitle_format_is_text(KivoSubtitleFormat format) noexcept;
bool subtitle_format_is_bitmap(KivoSubtitleFormat format) noexcept;
KivoSubtitleFormat subtitle_format_for_embedded_codec(
    KivoSubtitleEmbeddedCodecKind codec) noexcept;
KivoSubtitleTrackKind subtitle_track_kind_for_source(
    KivoSubtitleSourceKind source_kind,
    KivoSubtitleFormat format) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
