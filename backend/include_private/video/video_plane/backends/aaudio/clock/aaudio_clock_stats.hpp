#pragma once

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_trace.hpp"
#include <cstdint>

#include "video/video_plane/core/audio/clock/audio_clock_types.hpp"

namespace kivo::video::aaudio {

/// 一次音频时钟使用的完整事实计数。
///
/// 与 `AudioOutputStats` 同一约定：**只记事实，不含评价**。
/// 时钟精度够不够由证据层按 G7 判定，这里只如实记录走了哪条路、被拦了几次。
///
/// 为什么要区分 `timestamp_not_started` 和 `timestamp_fail`：
/// NDK 文档明确写着启动期返回 `INVALID_STATE` 是正常现象（"should not be considered
/// a fatal error, just try calling again later"）。把它算进失败，证据里会出现
/// 一堆"主源不可靠"的假象，而真出问题时反而看不出异常。
struct AaudioClockStats {
    std::int64_t queries = 0;

    // ── 主源 ──
    std::int64_t timestamp_ok = 0;
    std::int64_t timestamp_not_started = 0;  ///< 启动期，正常
    std::int64_t timestamp_fail = 0;         ///< 真的失败（不含 not_started）

    // ── 回退三档各用了几次 ──
    std::int64_t fallback_frames_read = 0;
    std::int64_t fallback_frames_written = 0;
    std::int64_t stale_freezes = 0;  ///< 三档都不可用，冻结在最后已知位置

    // ── 每一档真正**对外发布**了多少次 ──
    // 这是 `clock_source` 字段说实话的依据：只看"失败了几次"不够，
    // 关键是这次运行最后到底靠哪一档把时钟撑住的。
    std::int64_t published_timestamp = 0;
    std::int64_t published_frames_read = 0;
    std::int64_t published_frames_written = 0;
    std::int64_t published_frozen = 0;

    // ── 守卫 ──
    std::int64_t monotonic_rejects = 0;
    std::int64_t jumps_clamped = 0;

    /// 异常前后各 8 个原始观测（P14-E）。只有计数没有窗口时，
    /// "设备跳了"与"我们算错了"在证据里长得一样 —— 这正是本卡要分开的两种。
    ClockTrace trace{};

    // ── 启动观测（用于验证"零点 = 首帧实际出声"）──
    std::int64_t first_ok_frame_position = -1;  ///< 首次成功时的已呈现帧数
    std::int64_t first_ok_time_nanos = -1;      ///< 该帧对应的时钟刻度
    std::int64_t first_ok_mono_ns = -1;         ///< 首次成功时刻（同一把尺子）

    // ── 运行事实 ──
    std::int32_t sample_rate = 0;
    std::int64_t last_position_ns = 0;
    std::int64_t max_extrapolation_ns = 0;
    std::int64_t latency_frames = 0;  ///< 最近一次估出的 "已写入 − 已呈现"

    /// 最后一次实际发布时用的来源。全冻结时 `dominant_source()` 靠它兜底 ——
    /// 冻结发生后仍应报"上次那档坏了"，而不是谎报成主源。
    core::audio::AudioClockSourceKind last_source = core::audio::AudioClockSourceKind::kAudioTimestamp;

    [[nodiscard]] bool ever_ok() const noexcept { return timestamp_ok > 0; }

    /// 本次运行**主导**的时钟来源：发布次数最多的那一档。
    ///
    /// 证据里的 `clock_source` 必须用它，而不是"配置上打算用哪个"。
    /// 用主源撑住却报 `audio_timestamp` 是对的；靠回退撑住也报 `audio_timestamp`，
    /// 就是把"时钟精度已降级"这件事从证据里抹掉 —— G7 的 40 ms 恰恰依赖这一点。
    [[nodiscard]] core::audio::AudioClockSourceKind dominant_source() const noexcept {
        if (published_timestamp >= published_frames_read &&
            published_timestamp >= published_frames_written &&
            published_timestamp >= published_frozen) {
            return core::audio::AudioClockSourceKind::kAudioTimestamp;
        }
        if (published_frames_read >= published_frames_written &&
            published_frames_read >= published_frozen) {
            return core::audio::AudioClockSourceKind::kAudioFramesRead;
        }
        if (published_frames_written >= published_frozen) {
            return core::audio::AudioClockSourceKind::kAudioFramesWritten;
        }
        return last_source;
    }
};

}  // namespace kivo::video::aaudio
