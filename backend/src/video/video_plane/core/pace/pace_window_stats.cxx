#include "video/video_plane/core/pace/pace_window_stats.hpp"

#include <algorithm>
#include <array>

namespace kivo::video::core::pace {

std::int32_t split_windows(const DriftSeries& series,
                           std::int32_t window_count,
                           WindowSlice* out) noexcept {
    if (window_count <= 0 || out == nullptr) {
        return 0;
    }
    for (std::int32_t w = 0; w < window_count; ++w) {
        out[w] = WindowSlice{};
    }

    const std::int32_t n = series.size();
    if (n == 0) {
        return 0;
    }

    const std::int64_t t0 = series.at(0).media_ns;
    const std::int64_t span = series.at(n - 1).media_ns - t0;
    if (span <= 0) {
        // 时间不前进：所有点都算第 0 窗。不硬切成 window_count 份 ——
        // 跨度为零时每个点都落在边界上，切出来的结果由浮点/整除误差决定，
        // 那是"看起来分了窗"的假象。
        out[0].first = 0;
        out[0].count = n;
        return 1;
    }

    std::int32_t recorded = 0;
    for (std::int32_t i = 0; i < n; ++i) {
        const std::int64_t offset = series.at(i).media_ns - t0;
        std::int32_t index = static_cast<std::int32_t>(offset * window_count / span);
        // 末点归最后一窗：`offset == span` 时 `offset * window_count / span` 恰为 `window_count`。
        if (index >= window_count) {
            index = window_count - 1;
        }
        if (index < 0) {
            index = 0;
        }
        WindowSlice& slice = out[index];
        if (slice.count == 0) {
            slice.first = i;
            ++recorded;
        } else if (slice.first + slice.count != i) {
            // 点不连续 → 上面的"区间表示"前提被打破（媒体时刻倒退）。
            // 不修正、不修补：把这一窗降级成"只含从这里开始的一段"，
            // 让样本数与窗内实际点数对不上，判据 J1 会当场把它抓出来。
            // 静默合并才是真正危险的做法 —— 它会给出一个看起来正常的错中位数。
            continue;
        }
        ++slice.count;
    }
    return recorded;
}

WindowMedian median_of_slice(const DriftSeries& series,
                             WindowSlice slice,
                             SignedHistogram& fallback_histogram) noexcept {
    WindowMedian out;
    if (slice.count <= 0 || slice.first < 0) {
        return out;
    }
    out.samples = slice.count;

    if (slice.count <= kWindowScratchCapacity) {
        std::array<std::int64_t, kWindowScratchCapacity> scratch{};
        const std::int32_t n = slice.count;
        for (std::int32_t i = 0; i < n; ++i) {
            scratch[static_cast<std::size_t>(i)] = series.at(slice.first + i).lateness_ns;
        }
        // 秩取上整，与 `SignedHistogram::percentile_ns(0.50)` 同一口径：
        // rank = ceil(0.5 × n) = (n + 1) / 2（整数除法）。
        // 两份中位数必须能被同一套规则复算，否则"精确版"和"桶版"永远对不上，
        // 而证据里两者会同时出现（窗口分布 vs 整场分布）。
        //
        // 下面那行 `rank < 1` 的钳制看着多余（`(n + 1) / 2` 在 n ≥ 1 时恒 ≥ 1），
        // 但 `nth_element` 的第二个参数是 `begin() + (rank - 1)`，rank 为 0 时
        // 就是 `begin() - 1` —— MSVC 的调试迭代器会当场断言，Release 下是 UB。
        // 这条不是被当前实现踩出来的，是被**变异测试**踩出来的（C3 把秩改成
        // `n / 2`，单点窗立刻 `cannot seek array iterator before begin`）。
        // 变异该"红"，不该"崩"——崩了就没有信息量了。所以这里把下界钳住。
        const std::int32_t rank_raw = (n + 1) / 2;
        const std::int32_t rank = (rank_raw < 1) ? 1 : rank_raw;
        const auto nth = scratch.begin() + static_cast<std::ptrdiff_t>(rank - 1);
        std::nth_element(scratch.begin(), nth, scratch.begin() + n);
        out.value_ns = *nth;
        out.exact = true;
        return out;
    }

    // 超采样窗：退回桶中心。这条路在满容量单段序列上走不到（每窗 ≈ 256 点），
    // 但 seek 之后点可能挤在一窗里，必须有一条不会崩的路。
    fallback_histogram.clear();
    for (std::int32_t i = 0; i < slice.count; ++i) {
        fallback_histogram.add(series.at(slice.first + i).lateness_ns);
    }
    out.value_ns = fallback_histogram.percentile_ns(0.50);
    out.exact = false;
    return out;
}

}  // namespace kivo::video::core::pace
