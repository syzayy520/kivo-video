#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

void apply_close_tombstones(PlaneStateRecord& record) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
