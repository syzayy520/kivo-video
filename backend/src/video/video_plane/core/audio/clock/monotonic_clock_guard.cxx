#include "video/video_plane/core/audio/clock/monotonic_clock_guard.hpp"

#include <algorithm>

namespace kivo::video::core::audio {

MonotonicClockGuard::MonotonicClockGuard(std::int64_t jump_threshold_ns) noexcept
    : jump_threshold_ns_(jump_threshold_ns > 0 ? jump_threshold_ns : kDefaultJumpThresholdNs) {}

MonotonicClockGuard::Decision MonotonicClockGuard::observe(std::int64_t candidate_ns,
                                                          std::int64_t elapsed_ns) noexcept {
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

    // P14-H：判据从"位置前进了多少"改成"前进了多少**比本该前进的多多少**"。
    //
    // 旧写法 `advance > threshold` 隐含"查询间隔 ≪ 200 ms"。真机推翻了这个前提：
    // 片尾等 EOS 的一次 383 ms 间隔里位置也正常前进 383 ms（残差 ≈ 0），
    // 却被判成跳变钳掉 200 ms —— `jumps_clamped` 因此一直是噪声。
    //
    // 现在：这段时间最多"合理地"前进 `threshold + elapsed`，超出的部分才算跳变。
    const std::int64_t elapsed = (elapsed_ns > 0)
                                     ? std::min(elapsed_ns, kMaxElapsedNs)
                                     : std::int64_t{0};
    const std::int64_t allowance = jump_threshold_ns_ + elapsed;

    const std::int64_t advance = candidate_ns - max_ns_;
    if (advance > allowance) {
        ++jump_clamps_;
        // 放行的量也必须是 allowance（不是裸阈值）：否则"过了 383 ms 只放 200 ms"
        // 会凭空扣掉一段真实进度 —— 那正是本卡要消掉的那 183 ms。
        max_ns_ += allowance;
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
