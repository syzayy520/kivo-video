#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus copy_snapshot_font_blob(PlaneStateRecord& record,
                                        KivoSubtitleFontBlobId font_blob_id,
                                        KivoMutableByteSpan dst,
                                        uint64_t* out_required_bytes) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
