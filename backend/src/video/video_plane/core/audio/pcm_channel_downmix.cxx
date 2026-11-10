#include "video/video_plane/core/audio/pcm_channel_downmix.hpp"

#include <algorithm>

namespace kivo::video::core::audio {

std::int32_t downmix_5point1_to_stereo(const float* src, const std::int32_t src_frames, float* dst,
                                       const std::int32_t dst_capacity_frames) noexcept {
    if (src == nullptr || dst == nullptr || src_frames <= 0 || dst_capacity_frames <= 0) {
        return 0;
    }
    const std::int32_t frames = std::min(src_frames, dst_capacity_frames);
    for (std::int32_t i = 0; i < frames; ++i) {
        const float* in = src + static_cast<std::size_t>(i) * kSourceChannels5Point1;
        float* out = dst + static_cast<std::size_t>(i) * kStereoChannels;

        const float front_left = in[0];
        const float front_right = in[1];
        const float center = in[2];
        // in[3] = LFE：不进立体声。
        const float back_left = in[4];
        const float back_right = in[5];

        out[0] = front_left + kDownmixCenterWeight * center + kDownmixSurroundWeight * back_left;
        out[1] = front_right + kDownmixCenterWeight * center + kDownmixSurroundWeight * back_right;
    }
    return frames;
}

}  // namespace kivo::video::core::audio
