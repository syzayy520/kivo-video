#pragma once

#include <cstdint>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/pace/pace_action.hpp"
#include "video/video_plane/core/pace/pace_now.hpp"
#include "video/video_plane/core/pace/pace_policy.hpp"

namespace kivo::video::core::pace {

/// 把 `AudioClockSource` 变成"一个可信的、非递减的媒体位置"。
///
/// 它只做三件时钟本身不做的事：
/// 1. **陈旧外推**：`stale` 时按墙钟增量补位置（一个 burst 级）；
/// 2. **跳变重锚**：位置增量与墙钟增量严重不符时，改用墙钟增量推进；
/// 3. **倒退钳制**：位置变小一律不采纳（C4 的守卫已过滤一轮，这里是第二道）。
///
/// 它**不算** `due_at` —— 那是 `FramePacer` 的事。分开是因为这三件事
/// 每一条都要单独构造异常场景做断言，混进节拍器就没法单独测了。
class ClockPositionTracker {
public:
    struct Reading {
        /// 本次读取的单调时刻。
        std::int64_t mono_ns = 0;

        /// 修补后的媒体位置（绝对值，与 `AudioClockSource` 同一坐标系）。
        std::int64_t position_ns = 0;

        /// 相对锚点的已过时间。未锚定时为 0。
        ///
        /// 用它而不是 `position_ns - anchor_position`：
        /// 跳变时 `position_ns` 会跳，而这里已经吸收了跳变，保证连续。
        std::int64_t elapsed_ns = 0;

        ClockHealth health = ClockHealth::kFresh;
        core::audio::AudioClockSourceKind kind = core::audio::AudioClockSourceKind::kSystem;

        bool stale = false;
        bool jump_rebased = false;
        bool backward_clamped = false;
        bool monotonic_rejected = false;
    };

    ClockPositionTracker(core::audio::AudioClockSource& clock, const PacePolicy& policy,
                         NowFn now) noexcept;

    /// 读一次并推进内部位置。**每次呈现决策前恰好调一次**。
    [[nodiscard]] Reading read() noexcept;

    /// 用某次读取建立锚点（首帧）。锚点只建立一次；重复调用被忽略。
    void anchor(std::int64_t pts_ns, const Reading& reading) noexcept;

    [[nodiscard]] bool anchored() const noexcept { return anchored_; }
    [[nodiscard]] std::int64_t anchor_pts_ns() const noexcept { return anchor_pts_ns_; }

    /// 建锚那一刻的时钟位置（媒体时间轴上的绝对值）。
    ///
    /// C8 需要它：`av_drift` 是"相对锚点的漂移"，锚点建立时音频可能已经播了一段
    /// （AAudio 起流 + 首帧解码），那段常量恰好被锚点吸收。不把它单独报出来，
    /// 读者会把 `av_drift.p99` 当成全链路的音画偏差。
    [[nodiscard]] std::int64_t anchor_position_ns() const noexcept { return anchor_position_ns_; }

    /// seek / 换片源：丢掉锚点与历史，下次 `read()` 当首次处理。
    void reset() noexcept;

    // ---- 计数（进证据） ----
    [[nodiscard]] std::int64_t stale_readings() const noexcept { return stale_readings_; }
    [[nodiscard]] std::int64_t extrapolated_readings() const noexcept {
        return extrapolated_readings_;
    }
    [[nodiscard]] std::int64_t free_run_readings() const noexcept { return free_run_readings_; }
    [[nodiscard]] std::int64_t jump_rebases() const noexcept { return jump_rebases_; }
    [[nodiscard]] std::int64_t backward_clamps() const noexcept { return backward_clamps_; }
    [[nodiscard]] std::int64_t monotonic_rejects() const noexcept { return monotonic_rejects_; }
    [[nodiscard]] ClockHealth last_health() const noexcept { return last_health_; }
    [[nodiscard]] core::audio::AudioClockSourceKind last_kind() const noexcept { return last_kind_; }

private:
    core::audio::AudioClockSource& clock_;
    PacePolicy policy_;
    NowFn now_;

    bool has_previous_ = false;
    std::int64_t previous_mono_ns_ = 0;
    std::int64_t previous_raw_ns_ = 0;
    std::int64_t previous_position_ns_ = 0;  ///< 上一次修补后的位置，倒退钳制的基准
    std::int64_t position_ns_ = 0;
    std::int64_t stale_since_mono_ns_ = 0;  ///< 0 表示当前不陈旧

    bool anchored_ = false;
    std::int64_t anchor_pts_ns_ = 0;
    std::int64_t anchor_position_ns_ = 0;

    std::int64_t stale_readings_ = 0;
    std::int64_t extrapolated_readings_ = 0;
    std::int64_t free_run_readings_ = 0;
    std::int64_t jump_rebases_ = 0;
    std::int64_t backward_clamps_ = 0;
    std::int64_t monotonic_rejects_ = 0;
    ClockHealth last_health_ = ClockHealth::kFresh;
    core::audio::AudioClockSourceKind last_kind_ = core::audio::AudioClockSourceKind::kSystem;
};

}  // namespace kivo::video::core::pace
