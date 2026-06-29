#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus acquire_snapshot_lease(PlaneStateRecord& record,
                                       KivoSubtitleSnapshotLease* out_lease) noexcept;
KivoP8ApiStatus release_snapshot_lease(PlaneStateRecord& record,
                                       const KivoSubtitleSnapshotLease& lease) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
