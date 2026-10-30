#include "video/video_plane/core/audio/clock/monotonic_clock_guard.hpp"

#include <algorithm>

namespace kivo::video::core::audio {

MonotonicClockGuard::MonotonicClockGuard(std::int64_t jump_threshold_ns) noexcept
    : jump_threshold_ns_(jump_threshold_ns > 0 ? jump_threshold_ns : kDefaultJumpThresholdNs) {}

MonotonicClockGuard::Decision MonotonicClockGuard::observe(std::int64_t candidate_ns) noexcept {
    Decision decision;

    // 首次观测无条件接受：不能用 0 当基准，否则第一块数据若是负数
    // 或起始位置本来就非 0（流复用时），会被误判成倒退。
    if (!primed_) {
        primed_ = true;
        max_ns_ = candidate_ns;
        decision.position_ns = candidate_ns;
        return decision;
    }

    if (candidate_ns < max_ns_) {
        ++backward_rejects_;
        decision.position_ns = max_ns_;
        decision.rejected_backward = true;
        return decision;
    }

    const std::int64_t advance = candidate_ns - max_ns_;
    if (advance > jump_threshold_ns_) {
        ++jump_clamps_;
        max_ns_ += jump_threshold_ns_;
        decision.position_ns = max_ns_;
        decision.clamped_jump = true;
        return decision;
    }

    max_ns_ = candidate_ns;
    decision.position_ns = candidate_ns;
    return decision;
}

void MonotonicClockGuard::reset(std::int64_t position_ns) noexcept {
    max_ns_ = position_ns;
    primed_ = false;
}

}  // namespace kivo::video::core::audio
