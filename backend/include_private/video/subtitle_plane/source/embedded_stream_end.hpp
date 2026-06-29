#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitleCommandImmediateStatus apply_embedded_stream_end(
    PlaneStateRecord& plane,
    const KivoEmbeddedSubtitleStreamEndRequest& request) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
