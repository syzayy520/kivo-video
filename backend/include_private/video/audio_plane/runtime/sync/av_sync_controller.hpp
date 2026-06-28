// av_sync_controller.hpp — P6E: A/V sync controller
#pragma once

#include <cstdint>

namespace kivo::video::audio_plane::backend::runtime::sync {

// AvSyncController
// Computes A/V sync correction based on audio clock vs video clock.
class AvSyncController {
public:
    AvSyncController() = default;

    void set_audio_position_ms(int64_t audio_ms) noexcept { audio_ms_ = audio_ms; }
    void set_video_position_ms(int64_t video_ms) noexcept { video_ms_ = video_ms; }

    // Compute sync offset (positive = audio ahead, negative = video ahead).
    int64_t sync_offset_ms() const noexcept { return audio_ms_ - video_ms_; }

    // Returns true if sync is within threshold.
    bool is_in_sync(int64_t threshold_ms) const noexcept {
        int64_t offset = sync_offset_ms();
        return (offset >= -threshold_ms && offset <= threshold_ms);
    }

    // Returns correction suggestion: >0 = wait (audio ahead), <0 = skip (video ahead).
    int64_t correction_ms() const noexcept { return sync_offset_ms(); }

private:
    int64_t audio_ms_{0};
    int64_t video_ms_{0};
};

}  // namespace kivo::video::audio_plane::backend::runtime::sync
