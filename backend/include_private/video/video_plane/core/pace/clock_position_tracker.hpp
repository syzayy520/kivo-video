#pragma once

#include <cstdint>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/audio/clock/clock_rate_estimator.hpp"
#include "video/video_plane/core/pace/pace_discontinuity_ledger.hpp"
#include "video/video_plane/core/pace/pace_rebase_trace.hpp"
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
        /// 累积漂移超过 `drift_limit_ns`，这一帧按墙钟推进并把累积清零（P13-S）。
        bool drift_rebased = false;
        /// 这一帧实际修正了多少 ns（0 = 没修）。出版幅度才能分清
        /// "一次还完"（≈ 整个超出量）与"分很多笔还"（每笔很小）。
        std::int64_t drift_correction_ns = 0;
        bool backward_clamped = false;
        bool monotonic_rejected = false;

        // ---- P16-P1B11：频差观测（只算，不出版）----
        // 与 `ClockRateEstimator` 同一个低通估计；接线是为了让它在真实播放中每帧运行，
        // 出版要碰 `pace_stats.*`（他人主线），留到让出后。
        /// 低通后的速率偏差（ppm）。正数 = 音频钟比墙钟快。
        double rate_ppm = 0.0;
        /// 累计相位误差：位置 − 首样本位置 − 墙钟增量（与派生 lead 同式）。
        std::int64_t phase_error_ns = 0;
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

    /// P16-P1B32：pause 之后"跳过这段墙钟"，**不动**位置、锚点与漂移账。
    ///
    /// 为什么必须做：pause 期间视频线程停了、音频 timestamp 也冻住，
    /// resume 后第一次 `read()` 看到 `wall_delta = pause 时长`（实测 2.044 s）。
    /// 那一刻 timestamp 往往还是 stale，于是走**外推**分支
    /// `position_ns_ += wall_delta_ns` —— 位置凭空前进 2 秒，
    /// tracker 从此**永久超前**，视频为了追上丢掉 58 帧（实测值）。
    ///
    /// 做法只有一句：把上一帧的单调时刻前移到 now。这样 resume 后第一次读到的
    /// `wall_delta` 只剩 resume 之后那几毫秒，外推量回到正常范围。
    ///
    /// 不能改用 `reset()`：那会清锚点与 `cum_drift`，resume 首帧会被判迟到
    /// —— 正是 P1B29 引入 `preserve_stats` 要避免的。
    void skip_pause(std::int64_t now_ns) noexcept;

    // ---- 计数（进证据） ----
    [[nodiscard]] std::int64_t stale_readings() const noexcept { return stale_readings_; }
    [[nodiscard]] std::int64_t extrapolated_readings() const noexcept {
        return extrapolated_readings_;
    }
    [[nodiscard]] std::int64_t free_run_readings() const noexcept { return free_run_readings_; }
    [[nodiscard]] std::int64_t jump_rebases() const noexcept { return jump_rebases_; }
    /// 累积漂移触发的重锚次数（P13-S）。与 `jump_rebases` 分开数：
    /// 那个是"一次跳了 1 秒"，这个是"每次只差一点、攒到 45 ms"。
    [[nodiscard]] std::int64_t drift_rebases() const noexcept { return drift_rebases_; }
    /// 单次修正的最大幅度（P13-AC）。配合 `drift_rebases` 就能看出这一轮是
    /// "一次还完"（max ≈ total、次数 1）还是"分成多笔"（次数多、每笔小）。
    /// 它是诊断量，不是判据 —— 限速那版曾把它当判据，后来被真机否掉了。
    [[nodiscard]] std::int64_t drift_max_correction_ns() const noexcept {
        return drift_max_correction_ns_;
    }
    /// 累计修正量。收敛所需的"总账" —— 与 `drift_rebases × 单帧上限` 对照，
    /// 就能看出修正在几帧里摊完。
    [[nodiscard]] std::int64_t drift_corrected_total_ns() const noexcept {
        return drift_corrected_total_ns_;
    }
    /// 每次复位的时刻与幅度（P14-X）。回答"最大那一笔发生在什么时候" ——
    /// P14-W 里那次 105 ms 到底是启动期盲区的一次还清，还是别的东西。
    [[nodiscard]] RebaseTrace rebase_trace() const noexcept;
    /// 钟的不连续那本账（P14-Y）。回答"这些台阶一共吃掉多少领先量、最大那笔长什么样"。
    /// 门每帧从 `offset_ns()` 里减 —— 台阶不该被当成本事（漂移）还掉。
    [[nodiscard]] DiscontinuityTrace discontinuity_trace() const noexcept;
    [[nodiscard]] std::int64_t backward_clamps() const noexcept { return backward_clamps_; }
    [[nodiscard]] std::int64_t monotonic_rejects() const noexcept { return monotonic_rejects_; }
    [[nodiscard]] ClockHealth last_health() const noexcept { return last_health_; }
    [[nodiscard]] core::audio::AudioClockSourceKind last_kind() const noexcept { return last_kind_; }

private:
    /// 记一笔复位（P14-X）。只在 `read()` 的修正分支里调用。
    void record_rebase(std::int64_t mono_ns, std::int64_t amount_ns,
                       std::int64_t lead_before_ns) noexcept;

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
    /// 累积漂移（P13-S）：`Σ(位置增量 − 墙钟增量)`。它与 `previous_*` 不同寿命 ——
    /// 这个只被 `reset()` 清，跳变重锚不清它（跳变本身就是一次大漂移，清不清都行，
    /// 但只要它参与下一次判定，就必须与清零点同源）。
    std::int64_t cum_drift_ns_ = 0;
    /// 是否处在"收敛中"（P13-AC）。两个阈值：越过 `limit` 进入，降到
    /// `limit − band` 退出。只用一个阈值会让残差停在阈值下方一点（实测 44/45），
    /// 迟滞带就白设了。
    bool correcting_ = false;
    /// 首帧读取的单调时刻：用来划"启动期"（P13-W）。
    std::int64_t first_mono_ns_ = 0;
    /// 首帧的位置与墙钟原点（P13-AA）。领先量按 `position − (原点 + 墙钟增量)` 派生，
    /// 这样三条推进分支（正常 / stale 外推 / 跳变重锚）都被同一把尺子量到。
    std::int64_t origin_mono_ns_ = 0;
    std::int64_t origin_position_ns_ = 0;
    std::int64_t drift_rebases_ = 0;
    /// P13-AC：单帧修正的最大幅度与累计量。八个字节换"限速到底有没有生效"这个
    /// 可判定的问题 —— 只数次数是分不出"一帧削平"和"二十三帧摊平"的。
    std::int64_t drift_max_correction_ns_ = 0;
    std::int64_t drift_corrected_total_ns_ = 0;

    /// P16-P1B11：频差观测。只算、不参与任何判定 ——
    /// 接线的目的是让它在真实播放里每帧跑起来，为将来换速率伺服攒数据。
    core::audio::ClockRateEstimator rate_estimator_;

    // ---- 复位轨迹（P14-X）----
    //
    // 只留最近 8 笔 + 首笔 + 最大一笔。不做无界列表：两小时的会话会有上万个样本，
    // 而"最大那笔在什么时候"只需要三个数就能回答。
    std::int64_t rebase_first_at_ns_ = 0;
    RebaseSample rebase_max_{};
    RebaseSample rebase_recent_[8]{};
    std::int32_t rebase_head_ = 0;   ///< 下一笔写到哪
    std::int32_t rebase_count_ = 0;  ///< 环里已有几笔（≤ 8）
    std::int64_t backward_clamps_ = 0;
    std::int64_t monotonic_rejects_ = 0;
    /// 不连续那本账（P14-Y）。**单独一个类而不是再塞一个环**：这条文件已经 257 行，
    /// 两个环、两套分类、两处累计混在一起，改哪个都得先把整段读完。
    DiscontinuityLedger discontinuities_;
    ClockHealth last_health_ = ClockHealth::kFresh;
    core::audio::AudioClockSourceKind last_kind_ = core::audio::AudioClockSourceKind::kSystem;
};

}  // namespace kivo::video::core::pace
