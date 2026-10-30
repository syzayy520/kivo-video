#include "video/video_plane/core/pace/clock_position_tracker.hpp"

#include <algorithm>

namespace kivo::video::core::pace {

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
    anchored_ = false;
    anchor_pts_ns_ = 0;
    anchor_position_ns_ = 0;
}

}  // namespace kivo::video::core::pace
