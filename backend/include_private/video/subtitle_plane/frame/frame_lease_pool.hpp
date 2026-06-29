#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus acquire_frame_lease(PlaneStateRecord& record,
                                    const KivoSubtitleFrameBuildRequest& request,
                                    KivoSubtitleFrameLease* out_lease) noexcept;
KivoP8ApiStatus release_frame_lease(PlaneStateRecord& record,
                                    const KivoSubtitleFrameLease& lease) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
