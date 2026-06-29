#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

constexpr uint32_t kP8FrameBuildFlagSimulateHazardContention = 1u << 31u;

bool frame_hazard_retry_exhausted(const PlaneStateRecord& record,
                                  const KivoSubtitleFrameBuildRequest& request) noexcept;
bool eos_epsilon_expired(const PlaneStateRecord& record,
                         const KivoSubtitleFrameBuildRequest& request) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
