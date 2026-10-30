#include "video/video_plane/core/pace/pace_stats.hpp"

namespace kivo::video::core::pace {

void PaceStats::note_waited(std::int64_t wait_ns) noexcept {
    ++waited_;
    wait_total_ns_ += wait_ns;
}

void PaceStats::note_presented(std::int64_t lateness_ns) noexcept {
    ++presented_;
    ++latency_samples_;
    lateness_.add(lateness_ns);
}

void PaceStats::note_dropped(std::int64_t lateness_ns) noexcept {
    ++dropped_;
    dropped_lateness_.add(lateness_ns);
}

void PaceStats::note_catch_up_frame(std::int64_t lateness_ns) noexcept {
    ++dropped_;
    ++catch_up_frames_;
    dropped_lateness_.add(lateness_ns);
}

void PaceStats::note_interval(std::int64_t deviation_ns) noexcept {
    interval_.add(deviation_ns);
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
    out.clock_primary_frames = clock_primary_frames_;
    out.clock_fallback_frames = clock_fallback_frames_;
    out.clock_system_frames = clock_system_frames_;
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
    return out;
}

}  // namespace kivo::video::core::pace
