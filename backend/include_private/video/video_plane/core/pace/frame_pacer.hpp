#pragma once

#include <atomic>
#include <cstdint>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/pace/clock_position_tracker.hpp"
#include "video/video_plane/core/pace/pace_action.hpp"
#include "video/video_plane/core/pace/pace_now.hpp"
#include "video/video_plane/core/pace/pace_policy.hpp"
#include "video/video_plane/core/pace/pace_stats.hpp"

namespace kivo::video::core::pace {

/// 呈现节拍器：把 `pts` 翻译成"现在该不该呈现、睡多久、还是丢掉"。
///
/// 它**不碰** MediaCodec、不认识 `ANativeWindow`、不加锁、不分配（稳态）。
/// 调用方拿到 `kPresent` / `kWait` 之后自己决定怎么上屏（release 渲染 / 丢弃）。
///
/// 线程模型：非线程安全，只该在呈现线程里用。`note_presented()` 必须由同一线程调用。
class FramePacer {
public:
    struct Setup {
        /// 媒体位置来源。**必须非空**：没有时钟就没有节拍，纯视频时注入 `SystemClockSource`。
        core::audio::AudioClockSource* clock = nullptr;

        PacePolicy policy{};

        /// 便于单测替换；运行期用默认值。
        NowFn now = &monotonic_now_ns;
        WaitFn wait = &wait_until_ns;

        /// 停止标志。睡眠被切成小块并在块间检查它，所以停止不会卡满一个长睡。
        const std::atomic<bool>* stop_flag = nullptr;

        /// false = 只判定、不睡（P12-C7 的 `kSurfaceAtTime` 路径）。
        ///
        /// `arrive()` 仍然算 `due_at`，但跳过 `wait_until`，`decision.wait_ns` 恒为 0，
        /// 由调用方把 `due_at` 交给合成器去等。
        ///
        /// **默认 true —— C5 的行为一个字都不变。**
        /// 为什么不做成新的 `PaceAction`：`PaceAction` 回答"该不该呈现"，
        /// 睡不睡是"谁来等"的实现策略。混进枚举会让 C5 的状态机多一个分支，
        /// 而那个状态机是 C5 花了 5 轮真机才调稳的，不值得为省一个 bool 去动它。
        bool sleep_before_present = true;
    };

    explicit FramePacer(const Setup& setup) noexcept;

    /// 阈值自洽时为 true。为 false 说明配置写错了，调用方应拒绝启用节拍
    /// 而不是带着自相矛盾的阈值跑。
    [[nodiscard]] bool usable() const noexcept { return policy_.sane(); }

    /// 运行期改"kWait 时是否自睡"（P12-C7）。
    ///
    /// **只允许在开跑之前调用。** 它决定的是"谁来等"，中途改会让同一轮证据里
    /// 前半段的 `lateness` 与后半段的 `schedule_lead` 混在一起，两条直方图都没法解释。
    /// 之所以做成 setter 而不是构造参数：`attach_pacing` 与 `attach_pipeline` 的调用
    /// 顺序没有约定，构造期拿不到这个值。
    void set_sleep_before_present(bool enabled) noexcept { sleep_before_present_ = enabled; }

    /// 判定这一帧。返回 `kWait` 时**内部已经睡完**（睡到 `due_at`）。
    ///
    /// 每帧恰好调用一次，返回值可以只用于分支，不必保存；
    /// 但若呈现了，应当调 `note_presented()` 回填真实时刻，否则抖动样本会缺。
    [[nodiscard]] PaceDecision arrive(std::int64_t pts_ns) noexcept;

    /// 回填呈现时刻（`releaseOutputBuffer` 之前的瞬间）。
    ///
    /// 为什么不让 `arrive()` 自己记：它记的是"睡醒了"的时刻，
    /// 而真实呈现发生在调用方 `release` 之前的那一行。差几微秒，但那是**可观测**的几微秒，
    /// 由调用方给才诚实。
    void note_presented(const PaceDecision& decision, std::int64_t present_at_ns) noexcept;

    /// 回填"提交提前量"（P12-C7 的 `kSurfaceAtTime` 路径）。
    ///
    /// @param submit_at_ns 调用 `releaseOutputBufferAtTime` 之前的瞬间。
    ///
    /// 与 `note_presented()` 的分工：
    /// - `note_presented`：睡到点之后**迟了多少**，正常 ≈ 0；
    /// - `note_scheduled`：**提前多久**交出去，正常是负的固定值。
    ///
    /// 两者都会更新 `interval` 与 `frames_since_present_`（帧间隔规律性两条路径都要），
    /// 但只把偏移记进各自的直方图 —— 混记会让 `over_budget_frames` 把"正常提前"判成超预算。
    void note_scheduled(const PaceDecision& decision, std::int64_t submit_at_ns) noexcept;

    /// seek / 换片源 / 停再起：清掉锚点、历史与追帧状态。计数不清（证据要的是全会话）。
    ///
    /// 同时**切掉漂移序列的段**（P13-M）：锚点重建意味着 `lateness` 的零点换了，
    /// 跨过这一刻的回归拟合的是两个零点之间的跳变，不只是一条直线的速率差。
    /// 段数会 +1，但那是"段数"不是"计数" —— `presented` / `sampled_samples`
    /// / 各直方图一律不动。详见 `DriftSeries::break_segment()`。
    void reset() noexcept;

    [[nodiscard]] PaceSummary summary() const noexcept;

    [[nodiscard]] const ClockPositionTracker& tracker() const noexcept { return tracker_; }
    [[nodiscard]] std::int64_t frame_duration_ns() const noexcept { return frame_duration_ns_; }

private:
    void sleep_until(std::int64_t deadline_ns) noexcept;
    /// present 与 scheduled 两条记账路共用同一个 DueStep 构造（P14-AA）。
    /// 各写一遍的话，漏一格不会有任何判据报红 —— 跨度会悄悄变成 0。
    [[nodiscard]] DueStep make_due_step(const PaceDecision& decision,
                                        std::int64_t present_step_ns) const noexcept;
    [[nodiscard]] bool stop_requested() const noexcept;

    PacePolicy policy_;
    NowFn now_;
    WaitFn wait_;
    const std::atomic<bool>* stop_flag_ = nullptr;
    bool sleep_before_present_ = true;  ///< 见 `Setup::sleep_before_present`
    ClockPositionTracker tracker_;
    PaceStats stats_;

    bool has_last_pts_ = false;
    std::int64_t last_pts_ns_ = 0;

    bool has_last_due_ = false;
    std::int64_t last_due_ns_ = 0;

    bool has_last_present_ = false;
    std::int64_t last_present_ns_ = 0;
    std::int64_t frames_since_present_ = 0;

    std::int32_t consecutive_late_ = 0;
    bool catching_up_ = false;
    std::int64_t catch_up_events_ = 0;
    std::int64_t frames_decided_ = 0;

    /// 本帧 `arrive()` 读到的钟读数（P13-R）。`note_presented()` 在交出之后才被调用，
    /// 那时再读一次会得到另一个数 —— 记的是**决策那一刻**的钟，必须在 `arrive()` 里留。
    std::int64_t reading_mono_ns_ = 0;
    std::int64_t reading_elapsed_ns_ = 0;

    ClockHealth last_health_ = ClockHealth::kFresh;
    std::int64_t frame_duration_ns_ = 40'000'000;
};

}  // namespace kivo::video::core::pace
