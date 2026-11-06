#include "video/video_plane/core/pace/frame_pacer.hpp"

#include <cassert>

namespace kivo::video::core::pace {

FramePacer::FramePacer(const Setup& setup) noexcept
    : policy_(setup.policy),
      now_(setup.now != nullptr ? setup.now : &monotonic_now_ns),
      wait_(setup.wait != nullptr ? setup.wait : &wait_until_ns),
      stop_flag_(setup.stop_flag),
      tracker_(*setup.clock, setup.policy, now_) {
    sleep_before_present_ = setup.sleep_before_present;
    // 没有时钟就没有节拍。允许空指针等于允许"悄悄不节拍"，
    // 那种失败在证据里看不出来（jitter 全是 0），所以直接断死。
    assert(setup.clock != nullptr);
    frame_duration_ns_ = policy_.default_frame_duration_ns;
}

bool FramePacer::stop_requested() const noexcept {
    return stop_flag_ != nullptr && stop_flag_->load(std::memory_order_relaxed);
}

void FramePacer::sleep_until(std::int64_t deadline_ns) noexcept {
    // 切块睡：一次 `sleep_until` 睡 250ms 期间无法响应停止请求，
    // 那会让 stop() 出现可见卡顿。切块后最坏延迟一个块。
    for (;;) {
        const std::int64_t mono_ns = now_();
        const std::int64_t remaining_ns = deadline_ns - mono_ns;
        if (remaining_ns <= 0) {
            return;
        }
        if (stop_requested()) {
            return;
        }
        const std::int64_t chunk_ns = remaining_ns < policy_.max_single_wait_ns
                                          ? remaining_ns
                                          : policy_.max_single_wait_ns;
        wait_(mono_ns + chunk_ns);
    }
}

PaceDecision FramePacer::arrive(std::int64_t pts_ns) noexcept {
    PaceDecision decision;
    ++frames_decided_;

    // ---- 帧时长：只采纳"看起来像帧间隔"的 pts 差值 ----
    // 不信容器声明的 fps（07 §六 已禁按声明假设）。坏包、时间基切换给出的
    // 巨大/零差值在这里被挡掉，不会污染后续所有判定。
    if (has_last_pts_ && pts_ns > last_pts_ns_) {
        const std::int64_t delta_ns = pts_ns - last_pts_ns_;
        if (delta_ns >= policy_.min_frame_duration_ns &&
            delta_ns <= policy_.max_frame_duration_ns) {
            frame_duration_ns_ = delta_ns;
        }
    }

    const ClockPositionTracker::Reading reading = tracker_.read();
    if (!tracker_.anchored()) {
        // 首帧建立锚点：`due_at` 就是现在。启动/解码首帧的开销因此被排除，
        // 而不是被当成"迟到 130ms"丢掉（见 17-p12c5-design.md §2.3）。
        tracker_.anchor(pts_ns, reading);
    }
    stats_.note_health(reading.health);
    if (reading.drift_rebased) {
        stats_.note_drift_rebase(reading.drift_correction_ns);
    }

    const bool pts_discontinuous = has_last_pts_ && pts_ns <= last_pts_ns_;

    std::int64_t due_at_ns = 0;
    if (reading.health == ClockHealth::kFreeRun) {
        // 时钟停更过久：改用自生成时间轴，按帧率推进。这是显式降级，
        // 证据里 `free_run_frames` 非 0 就是在说"这轮的时间轴不是设备给的"。
        //
        // 刚降级的那一帧必须**从现在起算**：若接着用降级前的 last_due，
        // 时间轴会比现实落后一大截（陈旧了 500ms 以上），随后几十帧全被判迟到 →
        // 降级本身反而制造一场丢帧风暴。
        const bool entering_free_run = last_health_ != ClockHealth::kFreeRun;
        if (!has_last_due_ || entering_free_run) {
            due_at_ns = reading.mono_ns;
        } else {
            due_at_ns = last_due_ns_ + frame_duration_ns_;
        }
    } else if (pts_discontinuous) {
        // pts 重复或倒退：按一帧前进，绝不倒退。
        // 否则会出现"睡 −3ms"或两帧同时到期（坏片源、首帧重复都会触发）。
        due_at_ns = (has_last_due_ ? last_due_ns_ : reading.mono_ns) + frame_duration_ns_;
    } else {
        const std::int64_t media_delta_ns =
            pts_ns - tracker_.anchor_pts_ns() - reading.elapsed_ns;
        due_at_ns = reading.mono_ns + media_delta_ns;
    }

    const std::int64_t lateness_ns = reading.mono_ns - due_at_ns;

    // P13-P：目标时刻相对"上一帧 + 一帧"偏了多少。
    //
    // 这是把 `lateness` 拆成两半的那一半：`present` 侧由 `present_jitter` 回答，
    // `due` 侧只有这里知道（`last_due_ns_` 是节拍器私有的）。
    // 首帧没有"上一帧"—— 锚点就在这一帧建立，`due` 恒等于 `now`，比较无意义 ⇒ 记 0。
    const std::int64_t due_step_ns =
        has_last_due_ ? due_at_ns - (last_due_ns_ + frame_duration_ns_) : 0;

    const PaceVerdict verdict =
        decide(PaceInput{lateness_ns, consecutive_late_, catching_up_}, policy_);

    // P12-C8：记这一帧的时钟分档（与是否呈现无关，每帧恰好一次）。
    //
    // **这里不记 `av_drift`。** `lateness_ns` 在 `kSurfaceAtTime` 路径下等于提交
    // 提前量（实测 p50 = −99.875 ms = 3 帧），不是音画偏差。真正等于音画偏差的是
    // `note_presented()` 里的 `present_at − due_at`，也就是已有的 `lateness` 直方图。
    // 推导与实测见 `24-p12c8-design.md` §四之二。
    stats_.note_clock_kind(reading.kind);
    if (verdict.enter_catch_up) {
        catching_up_ = true;
        ++catch_up_events_;
    }
    if (verdict.leave_catch_up) {
        catching_up_ = false;
    }

    decision.action = verdict.action;
    decision.due_at_ns = due_at_ns;
    decision.lateness_ns = lateness_ns;
    decision.media_ns = pts_ns;
    decision.frame_duration_ns = frame_duration_ns_;
    decision.due_step_ns = due_step_ns;
    // P13-R：把这次读取的原始量留在成员上，`note_presented()` 在交出之后才被调用，
    // 那时已经读不到这一帧的 reading 了（再读一次会给另一个数）。
    reading_mono_ns_ = reading.mono_ns;
    reading_elapsed_ns_ = reading.elapsed_ns;
    decision.health = reading.health;

    switch (verdict.action) {
        case PaceAction::kWait: {
            if (sleep_before_present_) {
                decision.wait_ns = due_at_ns - reading.mono_ns;
                stats_.note_waited(decision.wait_ns);
                sleep_until(due_at_ns);
            } else {
                // 不睡：把该等的这段交给合成器（C7 的 AtTime 路径）。
                // wait_ns 保持 0 且不进 stats_ —— 否则 C5 的"平均等待"会被
                // 一条根本没等的路径稀释成无意义的数。
                decision.wait_ns = 0;
            }
            consecutive_late_ = 0;
            ++frames_since_present_;  // 即将被呈现，此刻先记一次、呈现后清零
            break;
        }
        case PaceAction::kPresent:
            consecutive_late_ = 0;
            ++frames_since_present_;
            break;
        case PaceAction::kDrop:
            ++consecutive_late_;
            ++frames_since_present_;
            decision.reason = "late beyond drop threshold";
            stats_.note_dropped(lateness_ns, pts_ns);
            break;
        case PaceAction::kCatchUp:
            ++consecutive_late_;
            ++frames_since_present_;
            decision.reason = "catching up after consecutive lateness";
            stats_.note_catch_up_frame(lateness_ns, pts_ns);
            break;
    }

    if (reading.health == ClockHealth::kFreeRun) {
        decision.reason = decision.reason[0] != '\0' ? decision.reason : "clock free run";
    }

    has_last_pts_ = true;
    last_pts_ns_ = pts_ns;
    has_last_due_ = true;
    last_due_ns_ = due_at_ns;
    last_health_ = reading.health;
    // P16-P1B12：把频差观测抄一份，供收帧线程交给采样器（只转发，不参与上面的决策）。
    last_rate_ppm_ = reading.rate_ppm;
    last_phase_error_ns_ = reading.phase_error_ns;
    // P16-P1B26：把当前位置发布给跨线程查询（`snapshot()` / UI 进度条）。
    position_ns_.store(reading.position_ns, std::memory_order_release);
    return decision;
}

void FramePacer::note_presented(const PaceDecision& decision,
                               std::int64_t present_at_ns) noexcept {
    // P13-Q：`present_step` 与 `note_interval()` 是同一个数（同一对时间戳、同一个期望），
    // 所以只算一次再两处用 —— 各算一遍迟早有一处忘了乘 `frames`。
    std::int64_t present_step_ns = 0;
    if (has_last_present_) {
        // 期望间隔 = 帧时长 × 中间经过的帧数（含当前帧，由 arrive() 每帧 +1）。
        // 中间丢过帧必须乘上去，否则"丢帧造成的 2 帧间隔"会被记成一帧抖动，
        // G6 的 p99 就被自己制造的假数据撑爆了。
        const std::int64_t frames = frames_since_present_ > 0 ? frames_since_present_ : 1;
        const std::int64_t expected_ns = decision.frame_duration_ns * frames;
        present_step_ns = (present_at_ns - last_present_ns_) - expected_ns;
    }

    stats_.note_presented(present_at_ns - decision.due_at_ns, decision.media_ns,
                          make_due_step(decision, present_step_ns));
    stats_.note_present_mono(present_at_ns);

    if (has_last_present_) {
        stats_.note_interval(present_step_ns);
    }
    last_present_ns_ = present_at_ns;
    has_last_present_ = true;
    frames_since_present_ = 0;
}

core::pace::DueStep FramePacer::make_due_step(const PaceDecision& decision,
                                               std::int64_t present_step_ns) const noexcept {
    // 两条记账路径共用同一个构造（P14-AA）：present 与 scheduled 若各写一遍，
    // 迟早有一处漏了 `anchor_pts_ns` 或 `due_at_ns`，而那种漏不会有任何判据报红 ——
    // 它只会让跨度悄悄变成 0。
    return DueStep{.due_step_ns = decision.due_step_ns,
                   .frame_duration_ns = decision.frame_duration_ns,
                   .present_step_ns = present_step_ns,
                   .clock_mono_ns = reading_mono_ns_,
                   .clock_elapsed_ns = reading_elapsed_ns_,
                   .anchor_pts_ns = tracker_.anchor_pts_ns(),
                   .due_at_ns = decision.due_at_ns};
}

void FramePacer::note_scheduled(const PaceDecision& decision,
                               std::int64_t submit_at_ns) noexcept {
    stats_.note_scheduled(submit_at_ns - decision.due_at_ns);
    // P14-AA：AtTime 档也要把 DueStep 喂进去，否则 `due_step_supplied = 0`、
    // 四个跨度全 0、`media_rate` 无从计算 —— 这一档以前就是这么没有量程的。
    // `present_step` 传 0：AtTime 档量到的是"提交间隔"，不是"交出的间隔"。
    // 给它编一个数才是更坏的做法 —— 那一格在这里没有定义。
    stats_.note_scheduled_step(make_due_step(decision, 0), decision.media_ns);
    stats_.note_present_mono(submit_at_ns);

    if (has_last_present_) {
        const std::int64_t frames = frames_since_present_ > 0 ? frames_since_present_ : 1;
        const std::int64_t expected_ns = decision.frame_duration_ns * frames;
        stats_.note_interval((submit_at_ns - last_present_ns_) - expected_ns);
    }
    last_present_ns_ = submit_at_ns;
    has_last_present_ = true;
    frames_since_present_ = 0;
}

void FramePacer::resume_at(std::int64_t now_ns) noexcept {
    // 只推"锚点"，不动 `frame_duration_` / 统计 / 帧计数。
    has_last_due_ = true;
    last_due_ns_ = now_ns;
    if (has_last_present_) {
        // 呈现锚点同理：不推的话 `note_interval` 会算出"停了 2 秒"的巨大间隔。
        last_present_ns_ = now_ns;
    }
    // pause 不该算进"连续迟到"，否则 resume 后立刻进追帧模式。
    consecutive_late_ = 0;
    catching_up_ = false;
    // P16-P1B32：跳过 pause 那段墙钟，并让频差观测重来。
    // 不做的话 resume 后第一次读的 `wall_delta` 就是 pause 时长，
    // stale 外推会把位置凭空推前 2 秒（实测 2.044 s → 丢 58 帧）。
    tracker_.skip_pause(now_ns);
}

void FramePacer::reset() noexcept {
    tracker_.reset();
    // 锚点没了 ⇒ `lateness = present_at − due_at` 的零点没了 ⇒ 漂移序列必须
    // 在这里切段（P13-M）。这是**唯一**知道"换了一条时间轴"的地方：
    // `DriftSeries::note()` 只能从 pts 推断，而 pts 前进推不出"没换时间轴"，
    // 于是向前 seek 那一次必然漏 —— 漏掉的那一次会把两个零点之间的跳变
    // 拟合成"漂移速率"，还会让 `media_span` 把两段之间十秒的空白算成播过的时长。
    //
    // 放在 `reset()` 里而不是 `seek_to()` 里：`begin_run()` 是本类唯一的
    // 起跑入口，`start()` / `seek_to()` / `resume_after_seek()` 都走它 ——
    // 三处各写一次迟早漏一处，而漏的那处不会有任何判据报红。
    stats_.note_anchor_reset();
    has_last_pts_ = false;
    last_pts_ns_ = 0;
    has_last_due_ = false;
    last_due_ns_ = 0;
    has_last_present_ = false;
    last_present_ns_ = 0;
    frames_since_present_ = 0;
    consecutive_late_ = 0;
    catching_up_ = false;
    last_health_ = ClockHealth::kFresh;
    last_rate_ppm_ = 0.0;
    last_phase_error_ns_ = 0;
    position_ns_.store(0, std::memory_order_release);
}

PaceSummary FramePacer::summary() const noexcept {
    PaceSummary out = stats_.summary(frame_duration_ns_, tracker_.last_health());
    out.frames_decided = frames_decided_;
    // P14-Q：这一格只能在这里算 —— `frames_decided_` 是节拍器私有的，
    // `PaceStats::summary()` 里它恒为 0，算出来的"未交付"会是 `−presented −dropped`。
    // 同一个理由让 P14-L 把 `catch_up_measured` 也放在这里。
    //
    // **不钳到 0**：负数意味着 `presented + dropped > frames_decided`，
    // 那是重复记账（同一帧被记了两次）—— 一个真缺陷。钳掉就把它藏起来，
    // 而判据 S3 正是拿这个符号当病征。让负数活着，让判据去红。
    out.undelivered_frames = frames_decided_ - out.presented - out.dropped;
    out.catch_up_events = catch_up_events_;
    // P14-L：`catch_up` 两格的可测性只能在这里判 —— `frames_decided` 是节拍器私有的，
    // `PaceStats::summary()` 看不到它（那里它恒为 0，判出来的永远是"没决策过"）。
    // 放在"确实知道"的那一层，是 P13-M `note_anchor_reset()` 的同一个做法。
    out.catch_up_measured = (frames_decided_ > 0) ? measured_ok()
                                                  : measured_missing(kNoDecisionsReason);
    out.jump_rebases = tracker_.jump_rebases();
    out.stale_readings = tracker_.stale_readings();
    out.backward_clamps = tracker_.backward_clamps();
    out.monotonic_rejects = tracker_.monotonic_rejects();
    out.clock_source = core::audio::audio_clock_source_name(tracker_.last_kind());
    // C8：建锚位置。`av_drift` 是相对量，被锚点吸收掉的常量必须能单独看到。
    out.anchor_pts_ns = tracker_.anchor_pts_ns();
    out.anchor_position_ns = tracker_.anchor_position_ns();
    // P13-AD：把门的配置一并出版。校验器据此复算，不再硬编码 45/5；
    // 关着门的对照组也不会被误读成"门没触发"。
    out.drift_rebase_trace = tracker_.rebase_trace();
    // P14-Y：不连续那本账也**照抄**，不在 `PaceStats` 里重算。
    // 算两次就是两个真相 —— `frame_worst` 与 `episode_` 的老账（P13-P）就是这么来的。
    out.drift_discontinuity = tracker_.discontinuity_trace();
    out.drift_gate_enabled = policy_.drift_gate_enabled;
    out.step_repay = policy_.step_repay;
    out.drift_limit_ns = policy_.drift_limit_ns;
    out.drift_band_ns = policy_.drift_band_ns;
    return out;
}

}  // namespace kivo::video::core::pace
