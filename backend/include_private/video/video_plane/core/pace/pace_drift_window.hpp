#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_histogram.hpp"

namespace kivo::video::core::pace {

/// **窗口**（序列里还留着的那些点）上的迟到分布。
///
/// 存在的理由是一条没人在意很久、但会让两张表打架的事实（P13-I）：
/// `PaceSummary::sampled_lateness` 是**整场累计**的（环形覆盖后继续加，
/// seek 之后也继续加），而 `DriftTrend` 描述的是**窗口**（封顶 2048 点 / 102.4 s）。
/// 两个数各自都说了实话，但它们是**两批不同的样本**。
///
/// 后果不是"某个数算错了"，是**读表的人会把它们当成同一批**：
/// 一份 130 秒的证据里 `p99` 覆盖 2600 个点、`slope` 覆盖 2048 个点，
/// 并排放着，谁也不会想起去看三行之外的 `truncated`。这与 P13-H 修掉的
/// "分子累计、分母窗口"是同一个病，只是那次在一条除法里，这次在两张表之间。
///
/// 修法照旧是结构性的：**让趋势自带分布**。`trend` 一旦带着自己那批点的
/// p50/p95/p99/max，读者就再也不需要跨块借数 —— 想比较"这段的抖动"和
/// "这段的斜率"，两个数在同一格里。
///
/// 与 `sampled_lateness` 的关系由判据 I4 钉死：**未覆盖且单段时，两份分布
/// 必须逐桶相同**（那时它们本来就是同一批点）；一旦不同，必须能指出原因
/// （`truncated` 或 `segments > 1`）。"不同"要有书面许可。
struct DriftWindow {
    /// 分布本身。桶映射与 `sampled_lateness` 完全一样，所以两者可以直接比桶。
    SignedHistogram lateness{};

    /// 进这份分布的样本数。**必须等于 `series.size()`**。
    ///
    /// 与 `SignedHistogram::count()` 并存是刻意的冗余：JSON 里要把这个数
    /// 写在分布**前面**（紧挨 `segment_accepted`），读者才能一眼看出
    /// "窗口 2048 / 本段 2600"这个差是怎么来的。分布自己的 `count` 在块内，
    /// 隔着一个 1024 桶的数组，指望读者翻过去核对是不现实的。
    std::int32_t samples = 0;
};

/// 从序列的当前窗口算一份分布。**纯函数**：不改序列、不读时钟、不分配。
///
/// 栈上 8 KB（一个 `SignedHistogram`）—— 与 `summarize_drift()` 里每算一个
/// 窗口中位数就要一个直方图是同一个量级，不是新增的负担。
[[nodiscard]] DriftWindow summarize_window(const DriftSeries& series) noexcept;

}  // namespace kivo::video::core::pace
