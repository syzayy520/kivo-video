#pragma once

#include <cstdint>

#include "video/video_plane/core/audio/clock/audio_clock_types.hpp"
#include "video/video_plane/core/pace/pace_action.hpp"
#include "video/video_plane/core/pace/pace_histogram.hpp"

namespace kivo::video::core::pace {

/// 一次节拍会话的度量结果。全部是事实计数与分布，不含评价。
///
/// 是否达标由证据层按 G6 判定，节拍器只负责如实记录。
struct PaceSummary {
    // ---- 决策计数 ----
    std::int64_t frames_decided = 0;
    std::int64_t presented = 0;      ///< kWait + kPresent
    std::int64_t dropped = 0;        ///< kDrop + kCatchUp
    std::int64_t catch_up_frames = 0;
    std::int64_t catch_up_events = 0;
    std::int64_t waited = 0;
    std::int64_t wait_total_ns = 0;

    // ---- 时钟健康度 ----
    std::int64_t extrapolated_frames = 0;
    std::int64_t free_run_frames = 0;
    std::int64_t jump_rebases = 0;
    std::int64_t stale_readings = 0;
    std::int64_t backward_clamps = 0;
    std::int64_t monotonic_rejects = 0;
    ClockHealth health = ClockHealth::kFresh;
    const char* clock_source = "unknown";

    // ---- 抖动分布 ----
    /// `present_at - due_at`（有符号，早为负）
    SignedHistogram lateness{};
    /// `(present[i] - present[i-1]) - frame_duration`
    SignedHistogram interval{};
    /// 被丢弃帧的迟到量。与 `lateness` 分开：混在一起会把 p99 抬到"丢帧的迟到"上，
    /// 而 G6 关心的是**实际呈现**的抖动。
    SignedHistogram dropped_lateness{};

    /// `submit_at - due_at`（P12-C7 的 `kSurfaceAtTime` 路径）：提交时刻相对目标显示时刻的偏移。
    ///
    /// 与 `lateness` 分开的理由：`lateness` 是"睡到点之后迟了多少"，正常应 ≈ 0；
    /// 而 `schedule_lead` 是"提前多久交出去"，正常应是**负的固定值**（提前量）。
    /// 两者混在一起会让 `over_budget_frames` 把"正常提前"判成"超预算"。
    ///
    /// 判稳定性看**跨度**（p1 到 p99），不看均值 —— 均值大不代表不稳。
    SignedHistogram schedule_lead{};

    /// 实际进了直方图的呈现样本数。小于 `presented` 说明调用方忘了回填时刻。
    std::int64_t latency_samples = 0;

    /// 进了 `schedule_lead` 直方图的样本数。`kCpuPaced` 路径恒为 0。
    std::int64_t scheduled_samples = 0;

    // ---- A/V 偏差（P12-C8）----
    //
    // 这里**刻意没有** `av_drift` 直方图。原因是一次真机测量推翻了最初的写法：
    //
    // `arrive()` 里的 `lateness_ns` 是"这一帧到达时相对到期时刻的偏移"，它只在
    // **CPU 睡到 `due_at`** 的路径上等于音画偏差。`kSurfaceAtTime` 路径不睡，
    // 于是连交 3 帧、`lateness` 恒等于提交提前量（实测 p50 = −99.875 ms = 3 帧，
    // 见 `p12c8-av-r1-sync.json`），跟音画同步没有半点关系。
    //
    // 真正等于音画偏差的量是 `note_presented()` 里的 `present_at − due_at`
    // （推导见 `24-p12c8-design.md` §四之二），也就是已有的 `lateness` 直方图。
    // 再存一份叫 `av_drift` 的会变成同一笔账记两处，早晚不一致。
    // 于是证据层直接从 `lateness` 换算，并由 `present_schedule` 判定能不能算。

    /// 逐帧记录的时钟分档（`AudioClockSourceKind`）。
    ///
    /// 为什么按帧数而不是按查询次数：G7 关心的是"呈现出来的画面有多少帧
    /// 是靠降级时钟对齐的"，不是"时钟内部重试了几次"。
    std::int64_t clock_primary_frames = 0;   ///< 主源（硬件时间戳）
    std::int64_t clock_fallback_frames = 0;  ///< 回退档（frames_read / frames_written）
    std::int64_t clock_system_frames = 0;    ///< 没有音轨，用的是独立单调时钟

    /// 建锚时刻的两侧位置（P12-C8）。被锚点吸收掉的常量在这里，不在 `av_drift` 里。
    std::int64_t anchor_pts_ns = 0;
    std::int64_t anchor_position_ns = 0;

    /// 首个/最后一个"交出去"的单调时刻（C8）。两者相减 = 呈现跨度（墙钟）。
    ///
    /// 有了它才能算播放速率：媒体时间走了多少 / 墙钟走了多少。
    /// 这是本项目唯一一个**不与节拍器结论循环**的检查 —— 它只看墙钟，
    /// 而 `av_drift`、`schedule_lead` 都是拿节拍器自己的目标时刻做基准。
    /// 速率掉到 0.5 或冲到 2.0 时，上面那些指标可能全都"完美"。
    ///
    /// 两条呈现路径共用：`note_presented` 与 `note_scheduled` 都会更新，
    /// 因为两者的时间戳都代表"这一帧按计划该出现了"的节奏。
    std::int64_t first_present_mono_ns = 0;
    std::int64_t last_present_mono_ns = 0;

    /// `|lateness| > frame_duration` 的帧数 —— G6 的"抖动是否超过一帧"直接看它。
    std::int64_t over_budget_frames = 0;

    // ---- 帧时长 ----
    std::int64_t frame_duration_ns = 0;
    double derived_fps = 0.0;

    // ---- 派生 ----
    double drop_ratio = 0.0;
};

/// 聚合器。**非线程安全** —— 只该在决策它的那个线程里用。
class PaceStats {
public:
    void note_waited(std::int64_t wait_ns) noexcept;
    void note_presented(std::int64_t lateness_ns) noexcept;
    void note_dropped(std::int64_t lateness_ns) noexcept;
    void note_catch_up_frame(std::int64_t lateness_ns) noexcept;
    void note_interval(std::int64_t deviation_ns) noexcept;
    void note_health(ClockHealth health) noexcept;

    /// 记一条"提交提前量"（`submit_at - due_at`，负表示提前）。仅 `kSurfaceAtTime` 路径用。
    void note_scheduled(std::int64_t lead_ns) noexcept;

    /// 记一帧的时钟分档。每帧调一次，与是否呈现无关。
    void note_clock_kind(core::audio::AudioClockSourceKind kind) noexcept;

    /// 记一次"交出去"的单调时刻（首帧写入，之后只更新末帧）。
    void note_present_mono(std::int64_t mono_ns) noexcept;

    [[nodiscard]] std::int64_t presented() const noexcept { return presented_; }
    [[nodiscard]] std::int64_t dropped() const noexcept { return dropped_; }

    [[nodiscard]] PaceSummary summary(std::int64_t frame_duration_ns,
                                      ClockHealth health) const noexcept;

private:
    std::int64_t presented_ = 0;
    std::int64_t dropped_ = 0;
    std::int64_t catch_up_frames_ = 0;
    std::int64_t waited_ = 0;
    std::int64_t wait_total_ns_ = 0;
    std::int64_t latency_samples_ = 0;
    std::int64_t extrapolated_frames_ = 0;
    std::int64_t free_run_frames_ = 0;
    SignedHistogram lateness_{};
    SignedHistogram interval_{};
    SignedHistogram dropped_lateness_{};
    SignedHistogram schedule_lead_{};
    std::int64_t scheduled_samples_ = 0;
    std::int64_t clock_primary_frames_ = 0;
    std::int64_t clock_fallback_frames_ = 0;
    std::int64_t clock_system_frames_ = 0;
    std::int64_t first_present_mono_ns_ = 0;
    std::int64_t last_present_mono_ns_ = 0;
    bool has_present_mono_ = false;
};

}  // namespace kivo::video::core::pace
