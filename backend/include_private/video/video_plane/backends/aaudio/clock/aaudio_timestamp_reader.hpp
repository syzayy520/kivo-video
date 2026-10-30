#pragma once

#include <aaudio/AAudio.h>

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_probe.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_timestamp_reading.hpp"

namespace kivo::video::aaudio {

/// 唯一调用 `AAudioStream_getTimestamp` 的地方。
///
/// 刻意做得极薄：只做"调 API + 翻译错误码"。所有会算错的东西
/// （外推、单调化、采样率切换）都在 `core/audio/clock/` 里，
/// 那些能在桌面机器上断言到底，这里不能。
class AaudioTimestampReader final : public AaudioClockProbe {
public:
    AaudioTimestampReader() = default;
    explicit AaudioTimestampReader(AAudioStream* stream) noexcept : stream_(stream) {}

    void bind(AAudioStream* stream) noexcept { stream_ = stream; }

    /// 读一次时间戳。不重试 —— 重试策略属于调用方，
    /// 因为"要不要等"取决于上层是在启动期还是稳态。
    [[nodiscard]] AaudioTimestampReading read_timestamp() const noexcept override;

    /// 回退档 1：已呈现帧数（与 timestamp 同坐标系、同语义，无需 latency 补偿）。
    /// 失败返回 -1。
    [[nodiscard]] std::int64_t read_frames_read() const noexcept override;

    /// 回退档 2：已写入帧数（比真实呈现位置**超前**一个缓冲深度，必须减 latency）。
    /// 失败返回 -1。
    [[nodiscard]] std::int64_t read_frames_written() const noexcept override;

    [[nodiscard]] bool bound() const noexcept { return stream_ != nullptr; }

private:
    AAudioStream* stream_ = nullptr;
};

}  // namespace kivo::video::aaudio
