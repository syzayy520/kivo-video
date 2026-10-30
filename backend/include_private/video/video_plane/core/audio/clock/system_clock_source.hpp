#pragma once

#include <cstdint>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"

namespace kivo::video::core::audio {

/// 无音轨时的时钟来源：一条独立的单调时间轴，从构造时刻起算。
///
/// **它必须自报 `kSystem`，不许冒充音频时钟。** 没有音轨就意味着没有任何
/// 设备呈现锚点，此时的时间轴只能"按墙钟走"；把它伪装成音频时钟，
/// 会让证据看起来像做成了 A/V 同步，实际上 G7 根本没有被测到。
///
/// 这里用 `std::chrono::steady_clock` 是刻意与 `FramePositionExtrapolator` 的规则不同：
/// 那边要求"与时间戳同源"（因为要和设备给的 `timeNanos` 相减），
/// 这边没有任何平台时间戳可对齐，steady_clock 就是唯一那把尺子。
class SystemClockSource final : public AudioClockSource {
public:
    SystemClockSource() noexcept;

    [[nodiscard]] AudioClockSnapshot now() override;

    /// 重新起算（换片源、恢复播放时把 0 点挪到现在）。
    void reset() noexcept;

    [[nodiscard]] std::int64_t origin_ns() const noexcept { return origin_ns_; }

private:
    std::int64_t origin_ns_ = 0;
};

}  // namespace kivo::video::core::audio
