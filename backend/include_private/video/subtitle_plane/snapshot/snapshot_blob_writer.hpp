#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

uint64_t snapshot_blob_required_bytes() noexcept;
KivoP8ApiStatus write_snapshot_blob(const PlaneStateRecord& record,
                                    KivoMutableByteSpan dst,
                                    uint64_t* out_required_bytes) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
