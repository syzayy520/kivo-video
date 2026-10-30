#include "video/video_plane/core/pace/clock_position_tracker.hpp"

#include <algorithm>

namespace kivo::video::core::pace {
namespace {

/// 单帧漂移超过这么多，就当成**台阶**而不是速率误差（P13-V）。
///
/// 为什么要这一条：P13-S 用"连续 60 帧正漂"来区分台阶与速率误差，真机撞墙了 ——
/// 8 路负载下 `clock_ahead +91.72 ms` 是**分多帧摊开**的（最差帧 `due_step` 只有 −3.8 µs），
/// 中间夹一次零漂就把连续计数清零，永远到不了 60（`97-p13u-status.md` §三）。
///
/// 改成按"单帧贡献"区分：60 fps 下 1.5% 的快钟每帧只多 250 µs，10% 也只有 1.7 ms，
/// 离 30 ms 差一个数量级；而设备重建 / 锚点重算的台阶通常在几十到几百毫秒。
/// 台阶归 `jump_limit_ns`（> 1 s）与基线吸收，不计入累积。
constexpr std::int64_t kStepDriftNs = 30'000'000;

/// 启动期多长之内的台阶不算数（P13-W）。首帧解码、Surface 首建、音频首包都会让
/// 时钟与墙钟在头一秒里对不齐，那不是"钟坏了"。C5-4/C5-5 那两条测试的台阶也落在这一段
/// （它们在第 1–2 帧就压 50 ms 进去），所以这条保护顺带把测试语义保住了。
constexpr std::int64_t kStepBlindPeriodNs = 1'000'000'000;

}  // namespace

ClockPositionTracker::ClockPositionTracker(core::audio::AudioClockSource& clock,
                                           const PacePolicy& policy, NowFn now) noexcept
    : clock_(clock), policy_(policy), now_(now != nullptr ? now : &monotonic_now_ns) {}

ClockPositionTracker::Reading ClockPositionTracker::read() noexcept {
    const std::int64_t mono_ns = now_();
    const core::audio::AudioClockSnapshot snapshot = clock_.now();

    Reading reading;
    reading.mono_ns = mono_ns;
    reading.stale = snapshot.stale;
    reading.monotonic_rejected = snapshot.monotonic_rejected;
    reading.kind = snapshot.source;
    if (snapshot.monotonic_rejected) {
        ++monotonic_rejects_;
    }

    if (!has_previous_) {
        // 首次：位置就是设备给的，没有历史可比，谈不上跳变。
        has_previous_ = true;
        first_mono_ns_ = mono_ns;
        origin_mono_ns_ = mono_ns;
        origin_position_ns_ = snapshot.position_ns;
        previous_mono_ns_ = mono_ns;
        previous_raw_ns_ = snapshot.position_ns;
        previous_position_ns_ = snapshot.position_ns;
        position_ns_ = snapshot.position_ns;
        if (snapshot.stale) {
            stale_since_mono_ns_ = mono_ns;
            ++stale_readings_;
        }
    } else {
        const std::int64_t wall_delta_ns = std::max<std::int64_t>(0, mono_ns - previous_mono_ns_);
        const std::int64_t raw_delta_ns = snapshot.position_ns - previous_raw_ns_;
        // P13-S：这一帧要减掉的超前量（0 = 不复位）。声明在分支外，
        // 因为减法必须发生在倒退钳制**之后**，而钳制在分支外。
        std::int64_t rebase_excess_ns = 0;
        (void)rebase_excess_ns;

        if (snapshot.stale) {
            ++stale_readings_;
            if (stale_since_mono_ns_ == 0) {
                stale_since_mono_ns_ = mono_ns;
            }
            const std::int64_t stale_for_ns = mono_ns - stale_since_mono_ns_;
            if (stale_for_ns >= policy_.stale_extrapolate_limit_ns) {
                // 停更太久：不再相信这个时钟。位置保持最后一次已知值，
                // 由上层切到自生成时间轴（见 FramePacer）。
                reading.health = ClockHealth::kFreeRun;
                ++free_run_readings_;
            } else {
                // 一个 burst 级的停更：按墙钟补。误差在微秒级，
                // 比"冻结位置让帧越等越晚"好得多。
                position_ns_ += wall_delta_ns;
                reading.health = ClockHealth::kExtrapolated;
                ++extrapolated_readings_;
            }
        } else {
            stale_since_mono_ns_ = 0;
            const std::int64_t drift_ns = raw_delta_ns - wall_delta_ns;
            if (drift_ns > policy_.jump_limit_ns || drift_ns < -policy_.jump_limit_ns) {
                // 位置一次跳了远超墙钟的量：换采样率 / seek / 设备重建。
                // 照原样用会让 due_at 整体平移，随后整段被判"迟到"→ 全丢。
                // 这一步改用墙钟增量，保持时间轴连续。
                position_ns_ += wall_delta_ns;
                reading.jump_rebased = true;
                ++jump_rebases_;
            } else {
                // P13-S：累积漂移门。`jump_limit_ns` 只管"一次跳 1 秒"，
                // 管不到"每帧只快 1.5%" —— 那种误差攒 8 秒就是 119 ms，
                // 而所有计数器都显示正常（真机实测，见 `91-p13r-status.md`）。
                // 位置按原始增量推进；领先量在钳制之后按**两端之差**统一算（P13-AA），
                // 这里不再做逐帧累积 —— 那本账漏掉了 stale / 跳变 / 钳制三条分支。
                position_ns_ += raw_delta_ns;
            }
            reading.health = ClockHealth::kFresh;
        }

        // 第二道倒退钳制：修补后的位置不允许变小。
        // 基准必须是"上一次修补后的位置"，不能是原始值 ——
        // 跳变重锚之后两者本就不同，拿原始值当基准会误判。
        if (position_ns_ < previous_position_ns_) {
            position_ns_ = previous_position_ns_;
            reading.backward_clamped = true;
            ++backward_clamps_;
        }

        // P13-AA：领先量改成**派生**，不再靠逐帧累加。
        //
        // 累积版只记正常分支，而 `stale` 外推 / `jump_rebase` / 倒退钳制都会直接
        // 把位置推到墙上 —— 那些推进不进账，于是真机出现 `clock_ahead +73.30 ms`
        // 而 `drift_rebases = 0`（`105-p13z-status.md` §二）。派生版按定义成立：
        //
        //     lead = position − (首帧位置 + 墙钟增量)
        //
        // 三条分支怎么推都不影响这条式子，因为它不从增量算，只从两端算。
        const std::int64_t wall_total_ns = mono_ns - origin_mono_ns_;
        cum_drift_ns_ = position_ns_ - (origin_position_ns_ + wall_total_ns);
        // 启动期不拦（P13-W 的约定）：首帧解码 / Surface 首建会让位置与墙在第一秒里
        // 对不齐，C5-4/C5-5 的 50 ms 台阶也落在这一段。
        const bool past_startup = mono_ns - origin_mono_ns_ > kStepBlindPeriodNs;
        // P13-AC：真正的迟滞是**两个阈值**，不是一个阈值加一个目标。
        //
        // 第一版写成"`cum > limit` 就修到 `limit − band`"，可每帧只修 2 ms，
        // 于是最后一帧修完落在 44 ms —— 已经低于触发阈值 45，状态机退出，
        // 永远到不了 40（真机四轮残差全是 44.0，正是这么来的）。
        // 改成：越过 `limit` 进入"收敛中"，一直修到 `<= limit − band` 才退出。
        if (past_startup && policy_.drift_gate_enabled) {
            if (!correcting_ && cum_drift_ns_ > policy_.drift_limit_ns) {
                correcting_ = true;
            }
        }
        if (past_startup && correcting_) {
            // P13-AC：**迟滞带**（限速已被真机否掉，见下）。
            //
            // 闭环的两个阈值：越过 `limit` 进入"收敛中"，修到 `limit − band` 才退出。
            //
            // 为什么要 band：P13-AB 里 `drift_rebases` 是 5/6/10，总修正量却只有
            // 35.2 ms —— 第一个复位吃掉绝大部分，其余全是"残差刚好等于门限、
            // 下一帧钟再挤 50 µs 就又超了"的微调。门限不能同时当触发阈值和收敛目标。
            //
            // 为什么没有单帧限速：第一版在这里加过"单帧最多 2 ms"，真机把它否掉了：
            //   事件帧 `due_step` 从 −45 变 −78（当帧只肯还 2 ms）、
            //   事件帧迟到从 29 变 62、200 ms 台阶还多丢 5 帧。
            // 而在**渐变漂移**上（八次真实事件的全部形状）触发时的超出量只有几毫秒，
            // 限速根本跑不满 —— 它只在"瞬时台阶"上起作用，且更差。
            // 所以撤掉限速，只留迟滞带：一次还完，但还到 `limit − band` 而不是 `limit`。
            const std::int64_t target_ns = policy_.drift_limit_ns - policy_.drift_band_ns;
            const std::int64_t correction_ns = cum_drift_ns_ - target_ns;
            if (correction_ns > 0) {
                position_ns_ -= correction_ns;
                cum_drift_ns_ -= correction_ns;
                reading.backward_clamped = false;
                reading.drift_rebased = true;
                reading.drift_correction_ns = correction_ns;
                ++drift_rebases_;
                drift_corrected_total_ns_ += correction_ns;
                if (correction_ns > drift_max_correction_ns_) {
                    drift_max_correction_ns_ = correction_ns;
                }
            }
            if (cum_drift_ns_ <= target_ns) {
                // 收敛完成。下一次触发要重新攒到 `limit`，中间这 `band` 就是迟滞带。
                correcting_ = false;
            }
        }
    }

    previous_mono_ns_ = mono_ns;
    previous_raw_ns_ = snapshot.position_ns;
    previous_position_ns_ = position_ns_;
    last_health_ = reading.health;
    last_kind_ = reading.kind;

    reading.position_ns = position_ns_;
    reading.elapsed_ns = anchored_ ? (position_ns_ - anchor_position_ns_) : 0;
    return reading;
}

void ClockPositionTracker::anchor(std::int64_t pts_ns, const Reading& reading) noexcept {
    if (anchored_) {
        return;
    }
    anchored_ = true;
    anchor_pts_ns_ = pts_ns;
    anchor_position_ns_ = reading.position_ns;
}

void ClockPositionTracker::reset() noexcept {
    has_previous_ = false;
    previous_mono_ns_ = 0;
    previous_raw_ns_ = 0;
    previous_position_ns_ = 0;
    position_ns_ = 0;
    stale_since_mono_ns_ = 0;
    cum_drift_ns_ = 0;
    correcting_ = false;
    anchored_ = false;
    anchor_pts_ns_ = 0;
    anchor_position_ns_ = 0;
}

}  // namespace kivo::video::core::pace
