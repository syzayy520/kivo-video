#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::mediacodec {

/// 节拍相关的度量，作为 `DecodeStats` 的一部分进证据。
///
/// 为什么不直接把 `core::pace::PaceSummary` 塞进来：那个结构里有三个 1024 桶的
/// 直方图（约 24KB）。证据 JSON 只需要分位数和计数，把直方图带进快照等于
/// 每次取样复制 24KB，而取样每秒要发生好几次。
struct DecodePaceStats {
    bool enabled = false;

    /// 真实驱动本次呈现的时钟（`audio_timestamp` / `audio_frames_read` / … / `system`）。
    /// 这一格必须说真话：纯视频时写 `system`，不许写 `audio_timestamp`。
    std::string clock_source = "none";

    /// 由观察到的 pts 差值推出的帧时长（不是容器声明）。
    std::int64_t frame_duration_ns = 0;
    double derived_fps = 0.0;

    // ---- 决策计数 ----
    std::int64_t frames_decided = 0;
    std::int64_t presented = 0;
    std::int64_t dropped = 0;
    double drop_ratio = 0.0;
    std::int64_t catch_up_events = 0;
    std::int64_t catch_up_frames = 0;

    // ---- 时钟健康度 ----
    std::int64_t extrapolated_frames = 0;  ///< 短暂 stale，按墙钟补过
    std::int64_t free_run_frames = 0;      ///< 停更过久，改用自走时间轴
    std::int64_t jump_rebases = 0;         ///< 时钟位置跳变被重锚的次数
    std::int64_t monotonic_rejects = 0;    ///< C4 单调守卫拒绝的次数（转发，不重复判定）
    std::int64_t stale_readings = 0;
    std::int64_t waited = 0;
    std::int64_t wait_total_ns = 0;

    // ---- 抖动（有符号：早为负、晚为正）----
    /// 进了 `lateness` 直方图的样本数。**必须出版**：`surface_at_time` 下
    /// `note_presented` 从不被调用，整块全零 —— 没有这一格，"零样本"会被读成"零延迟"。
    std::int64_t latency_samples = 0;

    std::int64_t lateness_p50_ns = 0;
    std::int64_t lateness_p95_ns = 0;
    std::int64_t lateness_p99_ns = 0;
    std::int64_t lateness_min_ns = 0;
    std::int64_t lateness_max_ns = 0;

    /// present 间隔偏差（相对"帧时长 × 中间经过帧数"）
    std::int64_t interval_p50_ns = 0;
    std::int64_t interval_p95_ns = 0;
    std::int64_t interval_p99_ns = 0;
    std::int64_t interval_max_ns = 0;

    /// `|lateness| > frame_duration` 的帧数。G6 的"抖动超过一帧"看它。
    std::int64_t over_budget_frames = 0;

    // ---- 提交提前量（P12-C7，仅 `surface_at_time` 路径有样本）----
    //
    // `submit_at - due_at`：负值 = 提前交出去。与上面的 `lateness` 语义相反，
    // 所以必须是独立一组字段 —— 合并会让"正常提前 3 帧"在 `over_budget_frames` 里
    // 变成"超预算"，读证据的人会以为 C7 把播放搞坏了。
    std::int64_t scheduled_samples = 0;
    std::int64_t schedule_lead_p01_ns = 0;
    std::int64_t schedule_lead_p10_ns = 0;
    std::int64_t schedule_lead_p50_ns = 0;
    std::int64_t schedule_lead_p90_ns = 0;
    std::int64_t schedule_lead_p99_ns = 0;
    std::int64_t schedule_lead_min_ns = 0;
    std::int64_t schedule_lead_max_ns = 0;

    /// 稳健跨度 `p90 − p10`。
    ///
    /// **不要用 `p99 − p01` 代替**：开播头几帧锚点刚建立，`due_at` 还没稳定，
    /// 提前量会是 0 / 半帧 / 一帧，这几帧只占 1–2%，正好落在 p01 之外、
    /// 却会把 p99 抬到 −30ms 附近，算出来的"跨度"变成 70ms —— 那是启动瞬态，
    /// 不是排程不稳。p10–p90 把这一段排除在外，测的是稳态。
    ///
    /// 瞬态本身没被藏起来：看 `min` / `max` 与 `scheduled_samples` 就知道头几帧长什么样。
    std::int64_t schedule_lead_spread_ns = 0;

    /// 快照是采样值，不是终值。取样发生在每 N 帧和会话结束时，
    /// 所以中途读到的数字会略微滞后 —— 这一格写清采样帧数，别让人以为是精确值。
    std::int64_t snapshot_at_frames = 0;

    /// 首/末次"交出去"的单调时刻（C8）。相减即呈现跨度，用来算播放速率。
    std::int64_t first_present_mono_ns = 0;
    std::int64_t last_present_mono_ns = 0;

    /// 逐帧时钟分档（P12-C8）。三者之和 = `frames_decided`（每帧恰好记一次）。
    std::int64_t clock_primary_frames = 0;   ///< 主源：硬件时间戳
    std::int64_t clock_fallback_frames = 0;  ///< 回退档（frames_read / frames_written）
    std::int64_t clock_system_frames = 0;    ///< 无音轨，独立单调时钟
};

}  // namespace kivo::video::mediacodec
