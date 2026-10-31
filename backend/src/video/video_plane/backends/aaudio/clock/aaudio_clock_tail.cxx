#include "video/video_plane/backends/aaudio/clock/aaudio_clock_tail.hpp"

#include <cstdlib>

namespace kivo::video::aaudio {

void ClockTailTracker::note(std::int64_t candidate_ns, std::int64_t mono_ns) noexcept {
    if (!primed_) {
        primed_ = true;
        last_candidate_ns_ = candidate_ns;
        last_mono_ns_ = mono_ns;
        return;
    }

    const std::int64_t candidate_step = candidate_ns - last_candidate_ns_;
    const std::int64_t mono_step = mono_ns - last_mono_ns_;
    last_candidate_ns_ = candidate_ns;
    last_mono_ns_ = mono_ns;

    // 间隔先记：它描述的是**查询循环**，与候选位置是否可信无关。
    // 放在下面的早退之后记，就会漏掉"候选倒退且间隔很长"这种最该看的组合。
    if (facts_.intervals == 0 && facts_.first_mono_ns == 0) {
        // `last_mono_ns_` 在本函数前半段已被本次观测覆盖，起点只能反推。
        facts_.first_mono_ns = mono_ns - mono_step;
    }
    if (mono_step > 0) {
        ++facts_.intervals;
        if (mono_step > facts_.max_mono_step_ns) {
            facts_.max_mono_step_ns = mono_step;
        }
        if (mono_step >= facts_.clamp_threshold_ns) {
            ++facts_.long_gaps;
            // 位置相对会话起点。起点在前面的分支里刚建立，这里一定有效。
            const std::int64_t at_ns = mono_ns - facts_.first_mono_ns;
            if (facts_.first_long_gap_at_ns < 0) {
                facts_.first_long_gap_at_ns = at_ns;
            }
            facts_.last_long_gap_at_ns = at_ns;
        }
    }

    // 候选步长独立记：它 = 间隔 + 残差，是守卫钳制的**充分条件**所在。
    // 放在这里（而不是只记残差）是因为首轮 30 分钟里有一次钳制，
    // 间隔与残差都不到阈值，两者之和却越了线。
    if (candidate_step > 0) {
        if (candidate_step > facts_.max_candidate_step_ns) {
            facts_.max_candidate_step_ns = candidate_step;
        }
        if (candidate_step >= facts_.clamp_threshold_ns) {
            ++facts_.long_candidate_steps;
        }
    }

    // 两个步长都必须为正才是一次可比较的观测。
    //
    // `mono_step <= 0` 说明拿不到墙钟（注入的假 now 停住了），此时算出来的
    // 残差是"设备步长"而不是"设备与墙钟之差" —— 混进尾部会把判据搞脏。
    // `candidate_step <= 0` 是真正的倒退事件，但它已经有专门的计数
    // （`monotonic_rejects`）和窗口（`trace`），不在这里重复记一遍：
    // 同一件事记两处，迟早两处对不上。
    if (mono_step <= 0 || candidate_step <= 0) {
        return;
    }

    const std::int64_t residual = candidate_step - mono_step;
    ++facts_.samples;

    if (residual >= 0) {
        ++facts_.forward;
        if (residual > facts_.max_forward_ns) {
            facts_.max_forward_ns = residual;
        }
    } else {
        ++facts_.backward;
        const std::int64_t magnitude = -residual;
        if (magnitude > facts_.max_backward_ns) {
            facts_.max_backward_ns = magnitude;
        }
    }

    // 跨度跟着走：最后一个观测的终点减去第一个观测的起点。
    facts_.span_ns = mono_ns - facts_.first_mono_ns;

    const std::int64_t magnitude = residual < 0 ? -residual : residual;
    std::int32_t bucket = 0;
    while (bucket < kClockTailBucketCount - 1 &&
           magnitude >= kClockTailEdgesNs[static_cast<std::size_t>(bucket)]) {
        ++bucket;
    }
    ++facts_.buckets[static_cast<std::size_t>(bucket)];

    // 越过阈值的计数**在这里判**，不从档位反推：档边界（128/256 ms）与
    // 阈值（200 ms）不重合，反推会漏掉 200–256 ms 那一批。
    if (magnitude >= facts_.clamp_threshold_ns) {
        ++facts_.beyond;
    }
}

void ClockTailTracker::drop_baseline() noexcept { primed_ = false; }

void ClockTailTracker::reset() noexcept {
    primed_ = false;
    last_candidate_ns_ = 0;
    last_mono_ns_ = 0;
    facts_ = ClockTailFacts{};
}

ClockTailFacts ClockTailTracker::snapshot() const noexcept { return facts_; }

}  // namespace kivo::video::aaudio
