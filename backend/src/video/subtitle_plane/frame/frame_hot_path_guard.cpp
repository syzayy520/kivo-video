#include "video/subtitle_plane/frame/frame_hot_path_guard.hpp"

namespace kivo::video::subtitle_plane::runtime {

bool frame_hazard_retry_exhausted(
    const PlaneStateRecord& record,
    const KivoSubtitleFrameBuildRequest& request) noexcept {
    if ((request.flags & kP8FrameBuildFlagSimulateHazardContention) == 0u) {
        return false;
    }
    const uint32_t budget = request.hazard_retry_budget == 0
        ? record.caps.rcu_hazard_acquire_max_retries
        : request.hazard_retry_budget;
    return budget <= record.caps.rcu_hazard_acquire_max_retries;
}

bool eos_epsilon_expired(
    const PlaneStateRecord& record,
    const KivoSubtitleFrameBuildRequest& request) noexcept {
    return record.state == KivoSubtitlePlaneState_EosReached &&
           request.playback_time.microseconds >
               static_cast<int64_t>(record.caps.subtitle_time_compare_epsilon_us);
}

}  // namespace kivo::video::subtitle_plane::runtime
