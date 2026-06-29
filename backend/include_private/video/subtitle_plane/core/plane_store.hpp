#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

PlaneStateRecord* create_plane_record(const KivoSubtitlePlaneRuntimeCaps& caps,
                                      KivoSubtitlePlaneHandle* out_handle) noexcept;
PlaneStateRecord* find_plane_record(KivoSubtitlePlaneHandle handle) noexcept;
bool destroy_plane_record(KivoSubtitlePlaneHandle handle) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
