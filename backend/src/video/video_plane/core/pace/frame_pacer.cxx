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
    decision.frame_duration_ns = frame_duration_ns_;
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
            stats_.note_dropped(lateness_ns);
            break;
        case PaceAction::kCatchUp:
            ++consecutive_late_;
            ++frames_since_present_;
            decision.reason = "catching up after consecutive lateness";
            stats_.note_catch_up_frame(lateness_ns);
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
    return decision;
}

void FramePacer::note_presented(const PaceDecision& decision,
                               std::int64_t present_at_ns) noexcept {
    stats_.note_presented(present_at_ns - decision.due_at_ns);
    stats_.note_present_mono(present_at_ns);

    if (has_last_present_) {
        // 期望间隔 = 帧时长 × 中间经过的帧数（含当前帧，由 arrive() 每帧 +1）。
        // 中间丢过帧必须乘上去，否则"丢帧造成的 2 帧间隔"会被记成一帧抖动，
        // G6 的 p99 就被自己制造的假数据撑爆了。
        const std::int64_t frames = frames_since_present_ > 0 ? frames_since_present_ : 1;
        const std::int64_t expected_ns = decision.frame_duration_ns * frames;
        stats_.note_interval((present_at_ns - last_present_ns_) - expected_ns);
    }
    last_present_ns_ = present_at_ns;
    has_last_present_ = true;
    frames_since_present_ = 0;
}

void FramePacer::note_scheduled(const PaceDecision& decision,
                               std::int64_t submit_at_ns) noexcept {
    stats_.note_scheduled(submit_at_ns - decision.due_at_ns);
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

void FramePacer::reset() noexcept {
    tracker_.reset();
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
}

PaceSummary FramePacer::summary() const noexcept {
    PaceSummary out = stats_.summary(frame_duration_ns_, tracker_.last_health());
    out.frames_decided = frames_decided_;
    out.catch_up_events = catch_up_events_;
    out.jump_rebases = tracker_.jump_rebases();
    out.stale_readings = tracker_.stale_readings();
    out.backward_clamps = tracker_.backward_clamps();
    out.monotonic_rejects = tracker_.monotonic_rejects();
    out.clock_source = core::audio::audio_clock_source_name(tracker_.last_kind());
    // C8：建锚位置。`av_drift` 是相对量，被锚点吸收掉的常量必须能单独看到。
    out.anchor_pts_ns = tracker_.anchor_pts_ns();
    out.anchor_position_ns = tracker_.anchor_position_ns();
    return out;
}

}  // namespace kivo::video::core::pace
