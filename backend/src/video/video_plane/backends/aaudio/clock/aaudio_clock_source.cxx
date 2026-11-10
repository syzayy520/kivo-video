#include "video/video_plane/backends/aaudio/clock/aaudio_clock_source.hpp"

#include <algorithm>

namespace kivo::video::aaudio {
namespace {

/// 回调 burst 与硬件时间戳发布周期不同。平板实测约 50ms 才发布一次，
/// 即使回调每 2ms 一次。容纳两个已观察周期；超过预算仍明确标 stale。
constexpr std::int64_t kTimestampAgeBudgetNs = 100'000'000;
constexpr std::int64_t kStaleBursts = 2;
/// P16-P1B2：仅作"其它调用方传 0"时的自卫（防非正陈旧阈值），产品路径不会走到 ——
/// AaudioOutputSession::open() 已在拿不到设备真值时明确失败并带 reason（G9）。
/// 用了它就必须能被 used_fallback() 查到，不许伪装成设备协商结果。
constexpr std::int32_t kDefaultFramesPerBurst = 192;  // 48 kHz 下 4 ms
constexpr std::int64_t kNanosPerSecond = 1'000'000'000;

[[nodiscard]] std::int64_t stale_threshold_ns(std::int32_t frames_per_burst,
                                              std::int32_t sample_rate) noexcept {
    return std::max(kTimestampAgeBudgetNs,
                    static_cast<std::int64_t>(frames_per_burst) * kStaleBursts *
                        kNanosPerSecond / sample_rate);
}

}  // namespace

AaudioClockSource::AaudioClockSource(AaudioClockProbe& probe,
                                     std::int32_t sample_rate,
                                     std::int32_t frames_per_burst,
                                     NowFn now_fn) noexcept
    : probe_(probe),
      now_fn_(now_fn != nullptr ? now_fn : &monotonic_now_ns),
      sample_rate_(sample_rate > 0 ? sample_rate : 48000),
      used_fallback_(frames_per_burst <= 0),
      frames_per_burst_(used_fallback_ ? kDefaultFramesPerBurst : frames_per_burst),
      stale_after_ns_(stale_threshold_ns(frames_per_burst_,
                                         sample_rate > 0 ? sample_rate : 48000)),
      extrapolator_(sample_rate > 0 ? sample_rate : 48000, stale_after_ns_) {
    stats_.sample_rate = sample_rate_;
    stats_.timestamp_age_budget_ns = stale_after_ns_;
}

const char* AaudioClockSource::fallback_reason() const noexcept {
    return used_fallback_
               ? "device frames_per_burst unavailable; fell back to 192 (48 kHz / 4 ms) "
                 "for stale-threshold safety only"
               : "device";
}

core::audio::AudioClockSnapshot AaudioClockSource::now() {
    ++stats_.queries;

    const AaudioTimestampReading reading = probe_.read_timestamp();

    if (reading.ok()) {
        ++stats_.timestamp_ok;

        const std::int64_t mono_ns = now_fn_();
        last_mono_ns_ = mono_ns;
        stats_.max_timestamp_age_ns =
            std::max(stats_.max_timestamp_age_ns, mono_ns - reading.time_nanos);
        if (stats_.first_ok_frame_position < 0) {
            stats_.first_ok_frame_position = reading.frame_position;
            stats_.first_ok_time_nanos = reading.time_nanos;
            stats_.first_ok_mono_ns = mono_ns;
        }

        // latency 估计：只在"已写入 ≥ 已呈现"时有意义。
        // 这个值只在回退档 2 用到 —— 那里拿到的 getFramesWritten 比真实呈现位置
        // 超前整整一个缓冲深度，不减掉它，时钟会突然跳前几十毫秒，
        // 同步器立刻判定视频落后并追帧，正好破坏 G7 要守的 40 ms。
        const std::int64_t written = probe_.read_frames_written();
        if (written >= 0 && written >= reading.frame_position) {
            latency_frames_ = written - reading.frame_position;
            stats_.latency_frames = latency_frames_;
        }

        core::audio::FramePositionExtrapolator::Sample sample;
        sample.frame_position = reading.frame_position;
        sample.time_nanos = reading.time_nanos;
        sample.sample_rate = sample_rate_;

        const auto extrapolated = extrapolator_.extrapolate(sample, mono_ns);
        if (extrapolated.stale) {
            ++stats_.timestamp_stale;
        }
        if (extrapolated.extrapolated_ns > stats_.max_extrapolation_ns) {
            stats_.max_extrapolation_ns = extrapolated.extrapolated_ns;
        }

        return publish(extrapolated.position_ns, core::audio::AudioClockSourceKind::kAudioTimestamp,
                       extrapolated.stale);
    }

    // 从来没成功过 + 还没进 STARTED = 启动期的正常现象，位置必然是 0。
    // 此时不走回退、不计失败：NDK 原文要求"just try calling again later"。
    //
    // 刻意**不喂给守卫**：若在这里 publish(0) 让守卫以 0 为基准，
    // 那么首个成功读数（启动开销期间设备可能已经播了几十毫秒）会被判成"跳变"并钳制，
    // 凭空砍掉一段真实进度。守卫的首次观测必须留给真正的第一个采样点。
    if (reading.status == AaudioTimestampStatus::kNotStarted && !stats_.ever_ok()) {
        ++stats_.timestamp_not_started;
        core::audio::AudioClockSnapshot snapshot;
        snapshot.position_ns = 0;
        snapshot.source = core::audio::AudioClockSourceKind::kAudioTimestamp;
        return snapshot;
    }

    // 已经成功过却失败，是真的失效了（流被停、设备被拔、实现抽风）。
    ++stats_.timestamp_fail;

    // 回退档 1：已呈现帧数。与 timestamp 同坐标系、同语义，无需任何补偿。
    // 这是把任务卡里的 `getFramesWritten` 降级的理由 —— 见 15-p12c4-design.md §五。
    const std::int64_t frames_read = probe_.read_frames_read();
    if (frames_read >= 0) {
        ++stats_.fallback_frames_read;
        last_mono_ns_ = now_fn_();
        return publish(frames_to_nanos(frames_read),
                       core::audio::AudioClockSourceKind::kAudioFramesRead, false);
    }

    // 回退档 2：已写入帧数减 latency。
    const std::int64_t frames_written = probe_.read_frames_written();
    if (frames_written >= 0) {
        ++stats_.fallback_frames_written;
        last_mono_ns_ = now_fn_();
        const std::int64_t presented = frames_written - latency_frames_;
        return publish(frames_to_nanos(presented > 0 ? presented : 0),
                       core::audio::AudioClockSourceKind::kAudioFramesWritten, false);
    }

    // 三档全部不可用：冻结在最后已知位置，明确标 stale。
    // 宁可让上层看到"时钟不动了"，也不许凭空造一个进度出来。
    ++stats_.stale_freezes;
    last_mono_ns_ = now_fn_();
    return freeze(last_kind_);
}

void AaudioClockSource::rebase(std::int64_t media_position_ns) noexcept {
    extrapolator_.rebase(media_position_ns);
    // 守卫必须同时重置：它记着 seek 之前的最大位置，不重置就会把新位置当成"倒退"丢掉。
    guard_.reset(media_position_ns);
    // 尾部只丢基线，**不清计数**：seek 让候选位置整体挪几秒到几分钟，
    // 旧基线配新位置会算出一个"等于 seek 距离"的残差，把尾部统计污染成假异常。
    // 而已经采到的样本仍然属于"本次运行"，没有理由抹掉。
    tail_.drop_baseline();
    // 守卫喂的是 elapsed，seek 之后也必须重设基线 —— 否则会把整段 seek 距离
    // 当成一次间隔送给守卫，"允许前进量"变成几秒，跳变判定等于关掉。
    has_last_publish_ = false;
}

void AaudioClockSource::set_sample_rate(std::int32_t sample_rate) noexcept {
    if (sample_rate <= 0 || sample_rate == sample_rate_) {
        return;
    }
    extrapolator_.set_sample_rate(sample_rate);
    sample_rate_ = sample_rate;

    // 保留发布年龄下限，同时重算 burst 对应的预算。
    stale_after_ns_ = stale_threshold_ns(frames_per_burst_, sample_rate_);
    extrapolator_.set_stale_after_ns(stale_after_ns_);

    stats_.sample_rate = sample_rate;
    stats_.timestamp_age_budget_ns = stale_after_ns_;
}

core::audio::AudioClockSnapshot AaudioClockSource::publish(std::int64_t candidate_ns,
                                                           core::audio::AudioClockSourceKind kind,
                                                           bool stale) {
    // P14-H：把"距上一次观测过了多久"告诉守卫。
    //
    // 这个 `elapsed` 必须与 `tail_` 用的间隔**同一个来源**（同一个 `last_mono_ns_`
    // 与同一个上次时刻），否则两处记账会分叉：证据说"间隔 383 ms"，
    // 守卫却按 33 ms 判 —— 那样修完还是误伤，只是没人看得出来。
    const std::int64_t elapsed_ns =
        has_last_publish_ ? (last_mono_ns_ - last_publish_mono_ns_) : 0;
    last_publish_mono_ns_ = last_mono_ns_;
    has_last_publish_ = true;

    const auto decision = guard_.observe(candidate_ns, elapsed_ns);
    if (decision.rejected_backward) {
        ++stats_.monotonic_rejects;
    }
    if (decision.clamped_jump) {
        ++stats_.jumps_clamped;
    }

    // P14-G：残差尾部。**在守卫之前算**用的是什么不重要，重要的是记的是
    // 设备原值 `candidate_ns` 与墙钟 `last_mono_ns_` 的步长之差 ——
    // 守卫改的是它的输出，不改这两个输入。
    tail_.note(candidate_ns, last_mono_ns_);
    stats_.tail = tail_.snapshot();

    // P14-E：把这次观测放进滚动窗口；异常时把窗口扣进证据。
    //
    // 为什么必须记 `candidate`（设备原值）而不只记 `accepted`（守卫输出）：
    // 守卫做的事就是**把它改掉**，只存改完的值，等于把证据留给唯一不该被信任的那一环。
    {
        ClockTraceSample sample;
        sample.mono_ns = last_mono_ns_;
        sample.candidate_ns = candidate_ns;
        sample.accepted_ns = decision.position_ns;
        sample.kind = static_cast<std::int32_t>(kind);
        sample.stale = stale ? 1 : 0;
        trace_ring_.push(sample);

        std::int32_t flags = 0;
        if (decision.rejected_backward) {
            flags |= 1;
        }
        if (decision.clamped_jump) {
            flags |= 2;
        }
        if (flags != 0) {
            trace_ring_.snapshot_into(stats_.trace, flags);
        }
    }

    stats_.last_position_ns = decision.position_ns;
    last_kind_ = kind;
    stats_.last_source = kind;

    switch (kind) {
        case core::audio::AudioClockSourceKind::kAudioTimestamp:
            ++stats_.published_timestamp;
            break;
        case core::audio::AudioClockSourceKind::kAudioFramesRead:
            ++stats_.published_frames_read;
            break;
        case core::audio::AudioClockSourceKind::kAudioFramesWritten:
            ++stats_.published_frames_written;
            break;
        case core::audio::AudioClockSourceKind::kSystem:
            break;
    }

    core::audio::AudioClockSnapshot snapshot;
    snapshot.position_ns = decision.position_ns;
    snapshot.source = kind;
    snapshot.stale = stale;
    snapshot.monotonic_rejected = decision.rejected_backward;
    snapshot.jump_clamped = decision.clamped_jump;
    return snapshot;
}

core::audio::AudioClockSnapshot AaudioClockSource::freeze(core::audio::AudioClockSourceKind kind) {
    ++stats_.published_frozen;

    core::audio::AudioClockSnapshot snapshot;
    snapshot.position_ns = guard_.max_position_ns();
    snapshot.source = kind;
    snapshot.stale = true;
    return snapshot;
}std::int64_t AaudioClockSource::frames_to_nanos(std::int64_t frames) const noexcept {
    const std::int32_t rate = (sample_rate_ > 0) ? sample_rate_ : 48000;
    const std::int64_t whole_seconds = frames / rate;
    const std::int64_t remainder = frames % rate;
    return whole_seconds * kNanosPerSecond + remainder * kNanosPerSecond / rate;
}

}  // namespace kivo::video::aaudio
