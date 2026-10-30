#pragma once

#include <cstdint>

#include "video/video_plane/backends/aaudio/session/aaudio_output_session.hpp"

namespace kivo::video::video_plane::probe {

/// 正弦波样本源。唯一用途：验证输出链路能把样本送到扬声器。
///
/// 刻意放在 probe 域而不是后端：`backends/aaudio` 只负责"把 PCM 交给设备"，
/// 一旦它开始自己造波形，后面接真实解码器时就得先把它拆掉。
class AaudioToneSource final : public aaudio::AaudioOutputSession::SampleSource {
public:
    AaudioToneSource(std::int32_t sample_rate,
                     std::int32_t channel_count,
                     double frequency_hz,
                     double amplitude);

    std::int32_t render(float* interleaved, std::int32_t frame_count) override;

private:
    std::int32_t channel_count_;
    double amplitude_;
    double phase_;           ///< 当前相位，[0, 2π)
    double phase_increment_; ///< 每帧相位增量 = 2πf / fs
};

}  // namespace kivo::video::video_plane::probe
