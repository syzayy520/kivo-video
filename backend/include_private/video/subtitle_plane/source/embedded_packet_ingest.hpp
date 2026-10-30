#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus ingest_embedded_packet(
    PlaneStateRecord& record,
    const KivoEmbeddedSubtitlePacketSubmitRequest& request,
    KivoSubtitleCommandToken* out_token) noexcept;

void store_memory_text_source(PlaneStateRecord& record,
                              KivoSubtitleInputBytesView bytes) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
