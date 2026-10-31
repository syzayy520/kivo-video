#include "video/video_plane/core/pace/present_gap.hpp"

namespace kivo::video::core::pace {

void PresentGapTracker::note(std::int64_t gap_ns,
                             std::int64_t submit_ns,
                             std::int64_t pump_ns,
                             std::int64_t pace_ns,
                             std::int64_t release_ns) noexcept {
    if (gap_ns <= 0) {
        return;  // 时钟没前进或倒退：这一圈不成立，不记
    }

    ++facts_.iterations;

    if (gap_ns >= 100'000'000) {
        ++facts_.long_gaps;
    }

    std::int32_t bucket = 0;
    while (bucket < kPresentGapBucketCount - 1 &&
           gap_ns >= kPresentGapEdgesNs[static_cast<std::size_t>(bucket)]) {
        ++bucket;
    }
    ++facts_.buckets[static_cast<std::size_t>(bucket)];

    if (gap_ns <= facts_.max_gap_ns) {
        return;  // 不是最差的一圈：三段不覆盖，只计数
    }

    facts_.max_gap_ns = gap_ns;
    facts_.worst_submit_ns = submit_ns > 0 ? submit_ns : 0;
    facts_.worst_pump_ns = pump_ns > 0 ? pump_ns : 0;
    facts_.worst_pace_ns = pace_ns > 0 ? pace_ns : 0;
    facts_.worst_release_ns = release_ns > 0 ? release_ns : 0;

    // 分段之和可能超过 gap：两段耗时各自用的 `SteadyClock` 读数与 gap 不同源、
    // 且相邻圈之间的边界归属有半拍误差。这时残差**必须是 0 而不是负数** ——
    // 一个负的"看不见的时间"会让读者以为我们量到了负时间。
    //
    // 注意只加 submit + pump：`pace` 与 `release` 是 pump 的**内部**，
    // 再加一次就等于把同一段时间算两遍。
    const std::int64_t accounted = facts_.worst_submit_ns + facts_.worst_pump_ns;
    facts_.worst_unaccounted_ns = gap_ns > accounted ? (gap_ns - accounted) : 0;
}

void PresentGapTracker::reset() noexcept { facts_ = PresentGapFacts{}; }

PresentGapFacts PresentGapTracker::snapshot() const noexcept { return facts_; }

}  // namespace kivo::video::core::pace
