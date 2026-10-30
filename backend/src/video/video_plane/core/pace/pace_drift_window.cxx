#include "video/video_plane/core/pace/pace_drift_window.hpp"

namespace kivo::video::core::pace {

DriftWindow summarize_window(const DriftSeries& series) noexcept {
    DriftWindow out;
    const std::int32_t n = series.size();
    for (std::int32_t i = 0; i < n; ++i) {
        out.lateness.add(series.at(i).lateness_ns);
    }
    // 用循环里的 `n` 而不是 `lateness.count()`：`SignedHistogram` 会把 ±128 ms
    // 之外的样本记进 `below` / `above` 溢出计数，**但 `count()` 照样包含它们**
    // （见 `pace_histogram.cxx`）。所以两者相等 —— 只是"相等"这件事该由构造保证，
    // 不该由一条断言去祈祷。`n` 是权威来源，`count()` 是它的回声。
    out.samples = n;
    return out;
}

}  // namespace kivo::video::core::pace
