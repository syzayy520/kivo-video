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
///
/// **P14-Y：这个常量在 P13-V 就被写下，但一直没人用** —— "台阶不计入累积"当时只是意图。
/// 现在它成了分界线：单帧偏离超过它就是 `kPositionStep`。30 ms 对 30 fps 接近一帧，
/// 而速率误差单帧只有几十微秒，差三个数量级。
constexpr std::int64_t kStepDriftNs = 30'000'000;

/// 启动期多长之内的台阶不算数（P13-W）。首帧解码、Surface 首建、音频首包都会让
/// 时钟与墙钟在头一秒里对不齐，那不是"钟坏了"。C5-4/C5-5 那两条测试的台阶也落在这一段
/// （它们在第 1–2 帧就压 50 ms 进去），所以这条保护顺带把测试语义保住了。
constexpr std::int64_t kStepBlindPeriodNs = 1'000'000'000;

/// 这一帧的不连续属于哪一类（P14-Y）。**一帧只认一类**，优先级从高到低 ——
/// 同一帧既被拒又被钳时记两笔，会把同一段墙时算两次，账直接翻倍。
///
/// 最后一类 `kPositionStep` 没有自己的标志位：它是"钟停了一下又补回来"的那一帧，
/// 落在正常分支里，只有比对位置增量与墙增量才看得见 —— P14-X 缺的 113 ms 就在这里。
DiscontinuityKind classify_discontinuity(const ClockPositionTracker::Reading& reading,
                                         std::int64_t step_ns, bool past_startup) noexcept {
    // 自由运行 = 已经不信这只钟了：位置是冻住的，领先量在这个档上没有意义，
    // 记进来只会每帧一笔把账冲垮（时钟停 10 s 就是 300 笔）。它归 `free_run_frames`。
    if (reading.health == ClockHealth::kFreeRun) {
        return DiscontinuityKind::kNone;
    }
    // 启动期（首帧后 1 s）不记账，与"启动期不拦"同一个约定（P13-W）：
    // 首帧解码 / Surface 首建 / 首包音频都会让位置与墙在第一秒里对不齐，
    // 那不是钟的不连续。这一段照旧留在原始领先量里，由门在盲区结束后处理。
    if (!past_startup) {
        return DiscontinuityKind::kNone;
    }
    if (reading.monotonic_rejected) {
        return DiscontinuityKind::kMonotonicRejected;
    }
    if (reading.backward_clamped) {
        return DiscontinuityKind::kBackwardClamped;
    }
    if (reading.jump_rebased) {
        return DiscontinuityKind::kJumpRebased;
    }
    if (reading.health == ClockHealth::kExtrapolated) {
        return DiscontinuityKind::kExtrapolated;
    }
    if (step_ns > kStepDriftNs || step_ns < -kStepDriftNs) {
        return DiscontinuityKind::kPositionStep;
    }
    return DiscontinuityKind::kNone;
}

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
        // P14-Y：这一帧开始时位置在哪。`step` 要的是"实际推进了多少"，
        // 而三条分支（外推 / 跳变 / 正常）各自推进方式不同，事后再算就算不清了。
        const std::int64_t position_before_ns = position_ns_;
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
                // P16-P1B32：估计器的基准必须跟着 rebase 一起重来。
                // 不重置的话 pause 那 2 秒会被记成"位置比墙钟少了 2 秒"，
                // 于是 `phase_error_ns` 一直挂着 -2 s（真机实测 -1,999,989,569）。
                // 这不是时钟的相位差，是"这一段没播"。
                rate_estimator_.reset();
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
        // 原始领先量：**不做任何扣除**。它就是 P14-T 以来一直出版的那个量，
        // 台阶摘不摘都在这里，读者永远能看到没被"加工"的那个数。
        const std::int64_t raw_lead_ns = position_ns_ - (origin_position_ns_ + wall_total_ns);

        // P14-Y：先认这一帧的台阶，再改 cum。
        //
        // `step = 位置实际推进 − 墙钟推进`。跳变与外推两条分支本来按墙推进，step 恒 0；
        // 停滞 / 钳制 / 补回来 这三类不是 0，而它们都不是速率误差 ——
        // 门把它们当漂移还掉，就是 P14-X 那笔 146.75 ms 的来源。
        const std::int64_t step_ns = (position_ns_ - position_before_ns) - wall_delta_ns;
        // 启动期不拦（P13-W 的约定）：首帧解码 / Surface 首建会让位置与墙在第一秒里
        // 对不齐，C5-4/C5-5 的 50 ms 台阶也落在这一段。记账与拦截共用同一个门。
        const bool past_startup = mono_ns - origin_mono_ns_ > kStepBlindPeriodNs;
        const DiscontinuityKind discontinuity =
            classify_discontinuity(reading, step_ns, past_startup);
        if (discontinuity != DiscontinuityKind::kNone) {
            // 记的是"这一帧之前 / 之后的原始领先量"，不是改过的 cum ——
            // 用改过的值反推 `before`，就是两处算同一个数（P14-X 的教训）。
            discontinuities_.note(discontinuity, mono_ns - first_mono_ns_, wall_delta_ns,
                                  raw_delta_ns, step_ns, raw_lead_ns - step_ns, raw_lead_ns);
        }
        // 门的输入：**默认仍是原始领先量**。
        //
        // P14-Z 把它做成探针（`step_repay`）：默认 `true` = 台阶进 `cum`、门照旧还它
        // （P13-W/S5 的契约）；`false` = 台阶先被摘掉，门不为它动作。
        // 两臂只差这一个开关，其余（门开、注入、片源）完全相同 —— 否则比的不是这件事。
        cum_drift_ns_ = policy_.step_repay
                            ? raw_lead_ns
                            : raw_lead_ns - discontinuities_.offset_ns();
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
                // P14-X：先把现场记下来，再改状态 —— 记完再改的话，
                // `lead_before` 就只能靠 `cum + correction` 反推，两处算同一个数。
                record_rebase(mono_ns, correction_ns, cum_drift_ns_);
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

    // P16-P1B11 / P16-P1B13：频差观测。**只算，不参与任何判定**。
    //
    // 关键：喂**原始位置** `snapshot.position_ns`，不能喂修正后的 `position_ns_`。
    // 漂移门每次复位都会把 `position_ns_` 减一截，若拿它算速率，
    // 估计器会把"门在修正"误读成"钟突然变慢" —— P1B13 注入 2000 ppm 时实测
    // `rate_ppm=-3123`、注入 5000 ppm 时 `-945`，既不匹配也不单调，就是这么来的。
    // 速率是**钟本身的属性**，必须在门动它之前测量。
    const auto rate = rate_estimator_.note({mono_ns, snapshot.position_ns});
    reading.rate_ppm = rate.rate_ppm;
    reading.phase_error_ns = rate.phase_error_ns;

    return reading;
}

void ClockPositionTracker::record_rebase(std::int64_t mono_ns, std::int64_t amount_ns,
                                         std::int64_t lead_before_ns) noexcept {
    RebaseSample sample{};
    // 相对首帧读数：绝对单调时钟在不同会话里差着开机时长，列出来没法读。
    sample.at_ns = mono_ns - first_mono_ns_;
    sample.amount_ns = amount_ns;
    sample.lead_before_ns = lead_before_ns;

    if (drift_rebases_ == 0) {
        rebase_first_at_ns_ = sample.at_ns;
    }
    if (sample.amount_ns > rebase_max_.amount_ns) {
        rebase_max_ = sample;
    }
    rebase_recent_[rebase_head_] = sample;
    rebase_head_ = (rebase_head_ + 1) % 8;
    if (rebase_count_ < 8) {
        ++rebase_count_;
    }
}

RebaseTrace ClockPositionTracker::rebase_trace() const noexcept {
    RebaseTrace out{};
    out.rebases = drift_rebases_;
    out.first_at_ns = rebase_first_at_ns_;
    out.max = rebase_max_;
    out.sampled = rebase_count_;
    out.samples_used = rebase_count_;
    // 环按时间升序展开：`rebase_head_` 是"下一笔写到哪"，也就是最旧那一笔的位置
    // （写满 8 笔之后）。没写满时最旧的是 0。
    const std::int32_t oldest = rebase_count_ < 8 ? 0 : rebase_head_;
    for (std::int32_t i = 0; i < rebase_count_; ++i) {
        out.recent[i] = rebase_recent_[(oldest + i) % 8];
    }
    return out;
}

DiscontinuityTrace ClockPositionTracker::discontinuity_trace() const noexcept {
    return discontinuities_.trace();
}

void ClockPositionTracker::anchor(std::int64_t pts_ns, const Reading& reading) noexcept {
    if (anchored_) {
        return;
    }
    anchored_ = true;
    anchor_pts_ns_ = pts_ns;
    anchor_position_ns_ = reading.position_ns;
}

void ClockPositionTracker::skip_pause(std::int64_t now_ns) noexcept {
    // 上一帧的时刻前移到 now —— pause 那一段墙钟不进 `wall_delta`，
    // 于是 stale 外推分支拿不到那 2 秒，位置不会被凭空推前。
    if (has_previous_) {
        previous_mono_ns_ = now_ns;
    }
    // pause 期间不该被算作"时钟停更"，否则 resume 后 stale 计时从 pause 起点算，
    // 一上来就可能越过 `stale_extrapolate_limit_ns` 直接进 FreeRun。
    stale_since_mono_ns_ = 0;
    // 频差观测重来：pause 的空白会被估计器读成"钟停了"，`phase_error` 会挂着 −2 s。
    rate_estimator_.reset();
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
    rebase_first_at_ns_ = 0;
    rebase_max_ = RebaseSample{};
    // 数组不能整体赋值（C3863）。逐个清 —— 只有 8 个，不值得为它引入 `std::fill`
    // 背后的头文件依赖（core/ 下刻意不用 `<algorithm>` 之外的容器设施）。
    for (RebaseSample& sample : rebase_recent_) {
        sample = RebaseSample{};
    }
    rebase_head_ = 0;
    rebase_count_ = 0;
    // P14-Y：不连续那本账与复位轨迹同寿命 —— 换片源之后，"上一段吃过多少台阶"
    // 不能接着算进这一段的门里。
    discontinuities_.reset();
    anchored_ = false;
    anchor_pts_ns_ = 0;
    anchor_position_ns_ = 0;
    // P16-P1B11：频差估计与位置同寿命 —— 换片源之后，"上一段快多少"不能接着算。
    rate_estimator_.reset();
}

}  // namespace kivo::video::core::pace
