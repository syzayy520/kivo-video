#pragma once

#include <cstdint>

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_probe.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_clock_tail.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_clock_trace.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_clock_stats.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.hpp"
#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/audio/clock/frame_position_extrapolator.hpp"
#include "video/video_plane/core/audio/clock/monotonic_clock_guard.hpp"

namespace kivo::video::aaudio {

/// AAudio 音频主时钟：读时间戳 → 外推 → 单调化 → 快照。
///
/// 核心不是"怎么读"，而是"用什么驱动"：位置一律由**已呈现帧数**换算，
/// 因此零点天然落在首帧真正出声的那一刻（见 15-p12c4-design.md §三）。
/// 不需要记录 `requestStart` 时刻，也不需要减任何延迟常数 ——
/// C1 实测启动开销 24–146 ms 波动，任何固定补偿都会错。
///
/// 读数通过 `AaudioClockProbe` 注入，`now` 通过函数指针注入：
/// 于是整个类零平台依赖，回退分级那三档能在桌面上被测到。
class AaudioClockSource final : public core::audio::AudioClockSource {
public:
    using NowFn = std::int64_t (*)() noexcept;

    /// `frames_per_burst` 用来定陈旧阈值（2 个 burst 还没更新就算卡住）。
    /// 传 0 或负数时退回 4 ms 的保守值。
    AaudioClockSource(AaudioClockProbe& probe,
                      std::int32_t sample_rate,
                      std::int32_t frames_per_burst,
                      NowFn now_fn = &monotonic_now_ns) noexcept;

    [[nodiscard]] core::audio::AudioClockSnapshot now() override;

    /// 切换采样率，位置保持连续（见 `FramePositionExtrapolator::set_sample_rate`）。
    void set_sample_rate(std::int32_t sample_rate) noexcept;

    /// seek 之后把时钟重设到新的媒体位置（P12-F2）。
    ///
    /// **两件事必须一起做**，缺一个都会出事：
    /// 1. 外推基准要改（否则位置从 0 起算）；
    /// 2. **单调守卫要 reset** —— 它的职责是"位置不许倒退"，而 seek 恰恰要倒退。
    ///    只改外推不改守卫，时钟会卡在 seek 之前的位置不动，视频被拖住、
    ///    所有计数却看起来正常 —— 这是本卡最容易漏的一步。
    void rebase(std::int64_t media_position_ns) noexcept override;

    [[nodiscard]] AaudioClockStats stats() const noexcept { return stats_; }
    [[nodiscard]] std::int32_t sample_rate() const noexcept { return sample_rate_; }

    /// P16-P1B2：这次构造是否用了 192 兜底。
    ///
    /// 产品路径不会走到 true —— `AaudioOutputSession::open()` 在拿不到设备真值时
    /// 已经明确失败（G9）。这里保留兜底只是为了防止探针 / 桌面对照传 0 时算出
    /// 非正的陈旧阈值；一旦用了，调用方必须能查到，不许"看起来像正常协商"。
    [[nodiscard]] bool used_fallback() const noexcept { return used_fallback_; }

    /// 非空、可读的兜底原因；未兜底时返回空串语义的 "device" 标记。
    [[nodiscard]] const char* fallback_reason() const noexcept;

private:
    [[nodiscard]] core::audio::AudioClockSnapshot publish(std::int64_t candidate_ns,
                                                          core::audio::AudioClockSourceKind kind,
                                                          bool stale);
    [[nodiscard]] core::audio::AudioClockSnapshot freeze(core::audio::AudioClockSourceKind kind);
    [[nodiscard]] std::int64_t frames_to_nanos(std::int64_t frames) const noexcept;

    AaudioClockProbe& probe_;
    NowFn now_fn_;
    std::int32_t sample_rate_;
    bool used_fallback_ = false;  ///< 必须先于 frames_per_burst_ 初始化（构造列表按声明顺序）
    std::int32_t frames_per_burst_;
    std::int64_t stale_after_ns_;

    core::audio::FramePositionExtrapolator extrapolator_;
    core::audio::MonotonicClockGuard guard_;
    AaudioClockStats stats_;

    std::int64_t latency_frames_ = 0;
    core::audio::AudioClockSourceKind last_kind_ = core::audio::AudioClockSourceKind::kAudioTimestamp;

    /// 最近一次 `now()` 里取到的墙钟时刻。`publish()` 拿不到它，
    /// 而窗口里的每一行都必须带"这是哪一刻的观测" —— 少了它，时间轴就断了。
    std::int64_t last_mono_ns_ = 0;
    /// 异常前后各 8 个原始观测的滚动窗口（P14-E）。
    ClockTraceRing trace_ring_;
    /// 残差尾部累加器（P14-G）。
    ClockTailTracker tail_;

    /// 上一次 `publish()` 的墙钟时刻，用来给守卫算 `elapsed`（P14-H）。
    /// `has_last_publish_ == false` 表示还没有上一次 —— 此时传 0，按最保守的判。
    std::int64_t last_publish_mono_ns_ = 0;
    bool has_last_publish_ = false;
};

}  // namespace kivo::video::aaudio
