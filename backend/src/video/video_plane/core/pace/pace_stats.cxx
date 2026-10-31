#include "video/video_plane/core/pace/pace_stats.hpp"

namespace kivo::video::core::pace {

void PaceStats::note_waited(std::int64_t wait_ns) noexcept {
    ++waited_;
    wait_total_ns_ += wait_ns;
}

void PaceStats::note_presented(std::int64_t lateness_ns, std::int64_t media_ns,
                               DueStep step) noexcept {
    ++presented_;
    ++latency_samples_;

    // P13-F/P13-P：逐帧极值与它的前后邻帧。
    //
    // 位置、极值、前因后果都在 `episode_` 里由**同一次比较**决定：上一版在
    // `PaceStats` 里另存一份 `frame_worst_`，那是同一个数写两处（先后差一位的
    // 经典错法就是那时踩出来的，注释留在 `WorstEpisodeRecorder` 里）。
    // 记录器自己数样本序号，所以下面两行的先后不影响 `index`。
    // P13-Q：两侧各自累积。`present_step` 由交出时刻的时间戳算（与 `present_jitter`
    // 同一个数），`due_step` 由钟算；两个来源不同，所以下面那条跨帧恒等式有内容。
    if (!has_lateness_) {
        has_lateness_ = true;
        first_lateness_ns_ = lateness_ns;
    }
    sum_present_step_ns_ += step.present_step_ns;
    sum_due_step_ns_ += step.due_step_ns;
    last_lateness_ns_ = lateness_ns;

    // P13-R：四个跨度。首个呈现帧定原点，之后每帧刷新末值 ——
    // 与累积账不同，这里只关心"整段走了多远"，不关心逐帧差。
    //
    // P14-U：**整段挪进了"有钟读数"的分支**。原来它在分支外面，于是
    // `surface_at_time` 或任何"第一帧还没钟"的轮次会把 0 当基准，
    // 算出的 `clock_lead_ns` 是 `last_elapsed − last_mono`（一个凭空多出几秒的数）。
    // 全帧都有读数的轮次行为**逐位不变**（每帧都进这个分支），
    // 所以 P14-T 那 110 份证据的数一个没动 —— 判据 U1 现在能要求逐位相等。
    if (step.due_at_ns != 0) {
        ++clock_supplied_frames_;
        note_span(step, media_ns);

        // P14-U：两个参考系。
        //
        // 门用的是 `due_at_ns != 0`，与上一行**同一个门**，不另立一套 ——
        // 两个门写在两处，迟早会在 `surface_at_time` 这种"只给一半"的档上分家。
        //
        // 为什么必须在这个分支里喂：那个档不带钟读数，把 480 个 0 喂进来会让
        // `max_step_ns = 0`、`verdict = steady` —— 一个漂亮、自信、完全假的观测。
        if (step.frame_duration_ns > 0) {
            lead_trace_.set_frame_duration(step.frame_duration_ns);
        }
        const std::int64_t lead_ns =
            lead_trace_.note(step.clock_elapsed_ns, step.clock_mono_ns);
        // `W = L − lead`。两个数各自独立测出来（一个来自交出时刻，一个来自钟与墙），
        // 所以这个减法有内容，不是同义反复。
        wall_offset_.note(lateness_ns - lead_ns, media_ns);
    }

    lateness_.add(lateness_ns);
    episode_.note(EpisodeSample{
        .lateness_ns = lateness_ns,
        .media_ns = media_ns,
        .due_step_ns = step.due_step_ns,
        .cumulative_present_step_ns = sum_present_step_ns_,
        .cumulative_due_step_ns = sum_due_step_ns_,
        .lateness_at_first_ns = first_lateness_ns_,
        .clock_mono_ns = step.clock_mono_ns,
        .clock_elapsed_ns = step.clock_elapsed_ns,
        .anchor_pts_ns = step.anchor_pts_ns,
        .due_at_ns = step.due_at_ns,
    });

    // P13-P：目标时刻动过几帧。判据是"超过一帧"—— 门限与 G6 的 over_budget 同口径，
    // 这样两个数能直接对照读（`due_step_frames` 与 `over_budget_frames` 不必相等：
    // 前者数的是**基准**偏了，后者数的是**结果**偏了；相等才需要解释）。
    note_due_step(step);

    // 抽稀门在同一处判定并存储：分两步（先问后加）会给两个真相。
    if (drift_.note(media_ns, lateness_ns)) {
        sampled_lateness_.add(lateness_ns);
        ++sampled_samples_;
    }
}

void PaceStats::note_anchor_reset() noexcept {
    // 锚点重建 = `lateness` 的零点换了。漂移序列必须在这里切段，而不是等
    // 下一帧的 pts 去推断 —— 推断只在"倒退"这一个方向上有效（P13-M）。
    //
    // 只动段，不动任何累计计数：`presented_` / `sampled_samples_` / 直方图
    // 回答的是"整场发生过什么"，与本卡无关。
    drift_.break_segment();
}

void PaceStats::note_dropped(std::int64_t lateness_ns, std::int64_t media_ns) noexcept {
    ++dropped_;
    dropped_lateness_.add(lateness_ns);
    drop_ring_.note(media_ns, lateness_ns);
}

void PaceStats::note_catch_up_frame(std::int64_t lateness_ns, std::int64_t media_ns) noexcept {
    ++dropped_;
    ++catch_up_frames_;
    dropped_lateness_.add(lateness_ns);
    drop_ring_.note(media_ns, lateness_ns);
}

void PaceStats::note_interval(std::int64_t deviation_ns) noexcept {
    interval_.add(deviation_ns);
}

void PaceStats::note_drift_rebase(std::int64_t correction_ns) noexcept {
    ++drift_rebases_;
    drift_corrected_total_ns_ += correction_ns;
    if (correction_ns > drift_max_correction_ns_) {
        drift_max_correction_ns_ = correction_ns;
    }
}

void PaceStats::note_health(ClockHealth health) noexcept {
    if (health == ClockHealth::kExtrapolated) {
        ++extrapolated_frames_;
    } else if (health == ClockHealth::kFreeRun) {
        ++free_run_frames_;
    }
}

void PaceStats::note_scheduled(std::int64_t lead_ns) noexcept {
    // 同时计入 presented_：AtTime 路径下这确实是一次"已呈现"的决定，
    // 只是它的时刻由合成器保证。不计数会让 drop_ratio 和 presented 看起来像没播。
    ++presented_;
    ++scheduled_samples_;
    schedule_lead_.add(lead_ns);
}

void PaceStats::note_scheduled_step(const DueStep& step, std::int64_t media_ns) noexcept {
    // P14-AA：AtTime 档的 DueStep 以前**从来没进过这本账** ——
    // `note_scheduled()` 只记提交提前量与间隔，于是 `due_step_supplied = 0`、
    // 四个跨度全 0、`media_rate` 无从计算。这一条路补的就是那两本账。
    //
    // **不碰 lateness / episode / 抽稀**：`present_at` 在 at_time 档是"提交时刻"，
    // 提前量 p50 ≈ −3 帧，写成 lateness 会把 G6 的 p99 撑爆 —— 那不是迟到。
    note_span(step, media_ns);
    note_due_step(step);
}

void PaceStats::note_span(const DueStep& step, std::int64_t media_ns) noexcept {
    // 跨度的唯一入口（P14-AA 抽出来的）。present 与 scheduled 都走这里 ——
    // 两处各写一遍，迟早有一处忘了改，而"跨度"这种两端之差最容易被写成两个真相。
    if (step.due_at_ns == 0) {
        return;  // 这一帧没有钟读数，不参与跨度
    }
    ++span_frames_;

    if (!has_span_) {
        has_span_ = true;
        first_arrive_mono_ns_ = step.clock_mono_ns;
        first_elapsed_ns_ = step.clock_elapsed_ns;
        first_due_ns_ = step.due_at_ns;
        first_pts_ns_ = media_ns;
    }
    last_arrive_mono_ns_ = step.clock_mono_ns;
    last_elapsed_ns_ = step.clock_elapsed_ns;
    last_due_ns_ = step.due_at_ns;
    last_pts_ns_ = media_ns;
}

void PaceStats::note_due_step(const DueStep& step) noexcept {
    if (step.frame_duration_ns > 0) {
        ++due_step_supplied_;
        const std::int64_t magnitude =
            step.due_step_ns < 0 ? -step.due_step_ns : step.due_step_ns;
        if (magnitude > step.frame_duration_ns) {
            ++due_step_frames_;
        }
    }
}

void PaceStats::note_clock_kind(core::audio::AudioClockSourceKind kind) noexcept {
    using core::audio::AudioClockSourceKind;
    switch (kind) {
        case AudioClockSourceKind::kAudioTimestamp:
            ++clock_primary_frames_;
            break;
        case AudioClockSourceKind::kAudioFramesRead:
        case AudioClockSourceKind::kAudioFramesWritten:
            ++clock_fallback_frames_;
            break;
        case AudioClockSourceKind::kSystem:
            ++clock_system_frames_;
            break;
    }
}

void PaceStats::note_present_mono(std::int64_t mono_ns) noexcept {
    if (!has_present_mono_) {
        has_present_mono_ = true;
        first_present_mono_ns_ = mono_ns;
    }
    last_present_mono_ns_ = mono_ns;
}

PaceSummary PaceStats::summary(std::int64_t frame_duration_ns, ClockHealth health) const noexcept {
    PaceSummary out;
    out.presented = presented_;
    out.dropped = dropped_;
    out.catch_up_frames = catch_up_frames_;
    out.waited = waited_;
    out.wait_total_ns = wait_total_ns_;
    out.extrapolated_frames = extrapolated_frames_;
    out.free_run_frames = free_run_frames_;
    out.latency_samples = latency_samples_;
    out.lateness = lateness_;
    out.interval = interval_;
    out.dropped_lateness = dropped_lateness_;
    out.schedule_lead = schedule_lead_;
    out.scheduled_samples = scheduled_samples_;
    out.sampled_lateness = sampled_lateness_;
    out.sampled_samples = sampled_samples_;
    out.drift_observed_frames = drift_.observed_frames();
    out.drift_skipped_frames = drift_.skipped_frames();
    out.drift_unobserved_cells = drift_.unobserved_cells();
    // P13-H：累计口径的跨度。`out.drift.span_ns` 是**窗口**跨度，两者不能混用 ——
    // 混用的那一处（报告的 `observed_hz`）正是本卡修的缺陷。
    out.drift_media_span_ns = drift_.media_span_ns();
    // P13-I：段内接受数。与 `drift_media_span_ns` 都是"本段/各段"口径，
    // 与 `sampled_samples` 的整场累计口径分开摆 —— 两个数在 seek 之后必然不等，
    // 把它们合成一个字段就是上一版 `truncated` 说假话的根源。
    out.drift_segment_samples = drift_.segment_samples();
    // P13-M：段内最大的那个洞。`drift_unobserved_cells` 是整场累计，
    // 答不出"最宽的洞有多宽"—— 而回归的可信度取决于最大值，不是总和。
    out.drift_segment_largest_gap_cells = drift_.largest_gap_cells();
    out.drift_segment_contiguous = drift_.contiguous();
    out.drift = summarize_drift(drift_);
    // P13-I：趋势自带的那批点，也自带它们的分布。
    // 这一行是"两张表合一"的全部实现 —— 它不做任何换算，只是把
    // `DriftTrend` 已经在回归的那批点再数一遍，于是两个数的样本必然同源。
    out.window_lateness = summarize_window(drift_);
    // P13-F：两个口径的最差点。抽稀那份由序列累计维护，`of` 已是全场累计数；
    // 逐帧那份的 `of` 只能在这里回填（见头文件注释）。
    out.sampled_worst = drift_.max_point();
    // P13-P：逐帧那份改为从 episode 派生 —— 极值与位置是同一次比较的结果，
    // `of` 与 `lateness_` 的样本数同源，不可能出现"index 说 480、samples 说 479"。
    out.frame_episode = episode_.episode();
    out.frame_episode.of = static_cast<std::int32_t>(lateness_.count());
    out.frame_worst = out.frame_episode.position();
    out.due_step_frames = due_step_frames_;
    out.due_step_supplied = due_step_supplied_;
    out.span_frames = span_frames_;
    // 哪条路喂的跨度：presented 有钟读数就是 presented；否则看 scheduled。
    // 两条都有（理论上不该发生）就报 mixed —— 让读者知道这一格不干净。
    if (clock_supplied_frames_ > 0 && span_frames_ > clock_supplied_frames_) {
        out.span_basis = "mixed";
    } else if (clock_supplied_frames_ > 0) {
        out.span_basis = "presented";
    } else if (span_frames_ > 0) {
        out.span_basis = "scheduled";
    } else {
        out.span_basis = "none";
    }
    // P13-Q：跨帧账。`closure_gap_ns` 是那条恒等式的残差 —— 出版它而不是断言它为 0，
    // 因为"记账中间漏了一帧"（丢帧 / 锚点重建）是真实会发生的事，读者需要看见。
    out.first_lateness_ns = first_lateness_ns_;
    out.last_lateness_ns = last_lateness_ns_;
    out.sum_present_step_ns = sum_present_step_ns_;
    out.sum_due_step_ns = sum_due_step_ns_;
    out.closure_gap_ns = (last_lateness_ns_ - first_lateness_ns_) -
                         (sum_present_step_ns_ - sum_due_step_ns_);
    // P13-R：四个跨度。正常分支上严格满足
    // `due_span == arrive_mono_span + pts_span − elapsed_span`。
    out.arrive_mono_span_ns = last_arrive_mono_ns_ - first_arrive_mono_ns_;
    out.elapsed_span_ns = last_elapsed_ns_ - first_elapsed_ns_;
    out.due_span_ns = last_due_ns_ - first_due_ns_;
    out.pts_span_ns = last_pts_ns_ - first_pts_ns_;
    // P14-T：钟相对墙的领先量。派生，不是测量 —— 但它必须在这里算，
    // 因为这是唯一同时握着两个跨度的层；渲染层只负责写出去。
    out.clock_lead_ns = out.elapsed_span_ns - out.arrive_mono_span_ns;
    out.clock_supplied_frames = clock_supplied_frames_;
    // P14-U：参考系的第二份。顺序有意为之 —— `span_ns` 从 `lead_trace` 出，
    // 上界又用它，所以 `lead_trace` 必须先算完。写成两行就不会有人调换。
    out.lead_trace = lead_trace_.summary();
    out.wall_offset = wall_offset_.summary();
    out.wall_reference_bound_ns =
        wall_reference_bound_ns(frame_duration_ns, out.lead_trace.span_ns);
    out.clock_primary_frames = clock_primary_frames_;
    out.clock_fallback_frames = clock_fallback_frames_;
    out.clock_system_frames = clock_system_frames_;
    // P13-S：漂移门的触发帧数。`FramePacer::summary()` 会把它连同 tracker 的四个计数
    // 一起交给证据层 —— 但**不是**从 tracker 取的（tracker 不知道自己被统计了几次）。
    out.drop_events = drop_ring_.trace();
    out.drift_rebases = drift_rebases_;
    out.drift_max_correction_ns = drift_max_correction_ns_;
    out.drift_corrected_total_ns = drift_corrected_total_ns_;
    out.first_present_mono_ns = first_present_mono_ns_;
    out.last_present_mono_ns = last_present_mono_ns_;
    out.frame_duration_ns = frame_duration_ns;
    out.health = health;

    if (frame_duration_ns > 0) {
        out.derived_fps =
            1'000'000'000.0 / static_cast<double>(frame_duration_ns);
    }
    const std::int64_t total = presented_ + dropped_;
    out.drop_ratio =
        total > 0 ? static_cast<double>(dropped_) / static_cast<double>(total) : 0.0;
    if (frame_duration_ns > 0) {
        out.over_budget_frames = lateness_.count_abs_beyond(frame_duration_ns);
    }
    // ── 派生量各自量到没有（P14-L）──
    //
    // 三格在这里填，第四格（`catch_up_measured`）在 `FramePacer::summary()`：
    // `frames_decided` 由节拍器记账，这个类看不到它。
    out.present_timestamp_measured = (latency_samples_ > 0)
                                         ? measured_ok()
                                         : measured_missing(kNoPresentTimestampReason);
    // `over_budget_frames` 是 `lateness_` 的派生量，所以它的可测性与上游**同源**。
    // 这里复制而不是重判：重判意味着"空直方图"这个条件被写在两个地方，
    // 而两处条件迟早会分家（P13-H 的 `observed_hz` 就是这么栽的）。
    out.over_budget_measured = out.present_timestamp_measured;
    out.drop_ratio_measured = (total > 0) ? measured_ok() : measured_missing(kNoDenominatorReason);
    return out;
}

}  // namespace kivo::video::core::pace
