#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_histogram.hpp"

namespace kivo::video::core::pace {

/// 一个窗口在序列里的下标区间 `[first, first + count)`。
///
/// 之所以能只用两个下标表示一窗，是因为 `DriftSeries` 里的点按媒体时刻
/// **单调递增**（seek 换时间轴时整条序列被清空重来，不会留下倒退的点）。
/// 这条前提一旦被打破，同一窗的点就不连续了 —— 到时候这里必须换成下标数组，
/// 而不是假装还能用区间。
struct WindowSlice {
    std::int32_t first = 0;
    std::int32_t count = 0;
};

/// 按媒体时间把序列切成 `window_count` 个等宽窗。
///
/// 边界规则与 `summarize_drift()` 里原来那段内联循环**逐字一致**：
/// `index = (media_ns − t0) × window_count / span`，末点归最后一窗。
/// 抽出来是因为中位数要从原始值选，"哪些点属于这一窗"必须只有一个答案 ——
/// 两处各写一遍，迟早在边界上分家（`span × w / window_count` 的整除截断
/// 是最容易写歪的地方）。
///
/// `out` 长度必须 ≥ `window_count`。返回实际有样本的窗数（≤ `window_count`）。
/// 空窗保持 `count == 0` 且**不压缩下标** —— 第 w 个槽永远对应第 w 窗，
/// 否则调用方拿 `recorded` 当窗号就会把起点算错。
[[nodiscard]] std::int32_t split_windows(const DriftSeries& series,
                                         std::int32_t window_count,
                                         WindowSlice* out) noexcept;

/// 一个窗口的中位数，以及"这个数是从原始值里选出来的，还是被量化过的"。
struct WindowMedian {
    std::int64_t value_ns = 0;
    std::int32_t samples = 0;

    /// `false` ⇒ 该窗样本数超过内部 scratch，退回 `SignedHistogram` 的桶中心。
    ///
    /// **退回之后 `value_ns` 就不能用来比较窗间差异了。** 桶宽 0.25 ms，
    /// 真机上 2013/2048 个点落在同一个桶里 —— 八个窗的中位数因此全是 125000，
    /// `delta_median_ns` 恒 0、`slope_from_windows_ppm` 恒 0。
    /// 那不是"没有漂移"，是"尺子上只有一个刻度"（见 `74-p13j-design.md` §一）。
    /// 所以这个标志必须一路传到证据里，读者才知道那个 0 该读成什么。
    bool exact = false;
};

/// 该窗的精确中位数（p50，秩取上整，与 `SignedHistogram::percentile_ns(0.50)` 同口径）。
///
/// 做法：把窗口内的**原始** `lateness_ns` 收进栈上 scratch，`std::nth_element` 选第 ⌈n/2⌉ 个。
/// 不用桶，理由是精度：`core/pace` 的漂移判据要在 130 s 上分辨 1 ppm，
/// 那对应中位数差 130 µs —— 而桶中心的量化误差是 ±125 µs，正好同量级。
/// **用桶版本，1 ppm 这个量级永远量不出来**，量出来的只有 0。
///
/// scratch 容量 `kScratchCapacity`（512 × 8 B = 4 KB 栈）。窗口点数通常是 `n/8`
/// （满容量时 256），够用；超过时退回桶中心并把 `exact` 置 false，由调用方
/// 连同窗口序号一起写进证据 —— 宁可说"这一窗是估的"，也不编一个精确的样子。
///
/// `fallback_histogram` 由调用方提供并复用（8 KB），避免每次进函数都占一块栈。
/// 只有在退回路径上才会被 `clear()` + 填充。
[[nodiscard]] WindowMedian median_of_slice(const DriftSeries& series,
                                           WindowSlice slice,
                                           SignedHistogram& fallback_histogram) noexcept;

/// scratch 容量，暴露出来只为让单测能精确踩到溢出边界。
inline constexpr std::int32_t kWindowScratchCapacity = 512;

}  // namespace kivo::video::core::pace
