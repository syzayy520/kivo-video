#include "video/video_plane/core/pace/pace_drift_series.hpp"

#include <algorithm>
#include <limits>

namespace kivo::video::core::pace {

void DriftSeries::begin_segment(std::int64_t media_ns) noexcept {
    has_last_ = true;
    last_media_ns_ = media_ns;
    segment_first_media_ns_ = media_ns;
    segment_last_accepted_ns_ = media_ns;  // 首帧必被接受，故起点即第一个接受点
    next_due_ns_ = media_ns + kSampleIntervalNs;
    // 这里**不**清 `segment_total_`：两处写同一个数早晚不一致。
    // 它的唯一写入点是 `clear()`（环形清空，计数跟着归零），而进入本函数的
    // 两条路都先过那里 —— 新建对象走默认值 0，seek 分支先 `clear()`。
}

void DriftSeries::close_segment() noexcept {
    if (!has_last_) {
        return;  // 没有段可关：会话还没开始，或刚被 `clear()` 关过
    }
    // 用**最后被接受**的那一帧，不用最后观察到的那一帧。
    // 尾部被抽稀门拒掉的几帧没有样本，算进跨度等于把没采到的时间算成采到了，
    // 而且会让这个数比窗口跨度大不到一帧 —— 两根尺子差一点点，比差很多更难发现。
    closed_span_ns_ += segment_last_accepted_ns_ - segment_first_media_ns_;
}

void DriftSeries::break_segment() noexcept {
    // 空会话上什么也不做：没有段可关，也不该把 `segments` 加成"发生过切段"。
    // 每个会话的 `start()` 都会走一次 `FramePacer::reset()` —— 那一次不能
    // 让每份证据都从 `segments = 2` 起步（那会把一个恒真的告警写进证据）。
    if (!has_last_) {
        return;
    }
    // `clear()` 里已经做了"先关段再清环形"，顺序不能反过来：
    // `close_segment()` 要的是 `segment_last_accepted_ns_`，而清环形之后
    // 那个数虽然还在，但读者会以为它来自 `at(size-1)` —— 两处取同一个量迟早分家。
    clear();
    ++segments_;
}

bool DriftSeries::note(std::int64_t media_ns, std::int64_t lateness_ns) noexcept {
    ++observed_;

    if (!has_last_) {
        // ① 首帧。不采首帧的话，短于 50 ms 的片源会得到空序列，
        //    而"空序列"和"没有漂移"在 JSON 里长得一样。
        begin_segment(media_ns);
    } else if (media_ns < last_media_ns_) {
        // ③ 兜底：pts 倒退**必然**是换了时间轴，但这一次没走 `break_segment()`
        //    （正常路径上 `FramePacer::reset()` 已经切过了，那时 `has_last_` 是 false，
        //    根本到不了这里）。留着是因为单测与将来的调用方可能没有 pacer。
        //
        //    注意这里**不能**用来推断"pts 前进 ⇒ 没换时间轴" —— 向前 seek 正是
        //    P13-L 那份缺陷证据：只看到前进，两段被拼成一段。
        break_segment();
        begin_segment(media_ns);
    } else if (media_ns < next_due_ns_) {
        // 还没跨过格点：这帧只更新"最晚见到的媒体时刻"，不占样本。
        ++skipped_;
        last_media_ns_ = media_ns;
        return false;
    } else {
        // ② 跨过了一个或多个格点。把 next_due 推到"下一个还没采过的格点"。
        //
        // 为什么要按格点推进而不是 `next_due = media + interval`：
        // 后者等价于"距上次接受够久就采"，30fps 会退化成 15 Hz（见头文件注释）。
        // 用绝对格点，误差不随帧率累积 —— 这正是本卡要的东西。
        const std::int64_t overdue_ns = media_ns - next_due_ns_;
        const std::int64_t cells = overdue_ns / kSampleIntervalNs;
        if (cells > 0) {
            // 这段媒体时间里没有任何帧跨过那些格点：记下来，但不补样本。
            unobserved_ += cells;
            // 段内最宽空洞（P13-M）。取 max 不累加：回归的可信度取决于
            // 最宽的那一个洞，十个 100 ms 的小洞不会让一条直线失去意义。
            // 钳到 int32：一次跳几十年的 pts 也不该让这个数溢出成负数。
            if (cells > largest_gap_cells_) {
                largest_gap_cells_ = static_cast<std::int32_t>(
                    std::min<std::int64_t>(cells, std::numeric_limits<std::int32_t>::max()));
            }
        }
        next_due_ns_ += (cells + 1) * kSampleIntervalNs;
        last_media_ns_ = media_ns;
    }

    points_[static_cast<std::size_t>(write_index_)] = DriftPoint{media_ns, lateness_ns};
    write_index_ = (write_index_ + 1) % kCapacity;
    if (size_ < kCapacity) {
        ++size_;
    }
    ++total_;
    ++segment_total_;
    segment_last_accepted_ns_ = media_ns;

    // P13-F：极值累计维护（O(1)）。放在 `++total_` 之后，
    // 这样 `index` 天然是"全场第几个"，与 `at()` 的窗口内下标无关。
    //
    // 严格大于：并列时保留时间上最早的（先到先得），遍历方向变了结论也不变。
    // `clear()` 不清它 —— 与 `total_`、`segments_` 同一条道理：
    // 证据要的是"整场会话发生过什么"，不是"此刻缓冲里有什么"。
    if (!worst_.valid || lateness_ns > worst_.lateness_ns) {
        worst_.valid = true;
        worst_.index = static_cast<std::int32_t>(total_ - 1);
        worst_.media_ns = media_ns;
        worst_.lateness_ns = lateness_ns;
    }
    return true;
}

void DriftSeries::clear() noexcept {
    // 关段在前：`size_ = 0` 之后 `at()` 就取不到末点了，
    // 而 `close_segment()` 要的是 `last_media_ns_`（它不受 `size_` 影响，但仍按"先关后清"写，
    // 免得将来有人把末点改成从序列里取）。
    close_segment();
    size_ = 0;
    write_index_ = 0;
    // 段内计数跟着环形一起归零（P13-I）。它是**唯一**的写入点 —— 在 `begin_segment()`
    // 里再清一次就是两处写同一个数，早晚不一致。
    //
    // 归零让 `size_ == min(segment_total_, kCapacity)` 在**任何时刻**都成立，
    // 包括"clear 之后还没进新帧"这个空活段的瞬间。不归零的话，那一刻会同时出现
    // `size() == 0` 与 `truncated() == true` —— 正是本卡要消灭的那种自相矛盾。
    segment_total_ = 0;
    // 段内最宽空洞跟着环形一起归零（P13-M）。与 `segment_total_` 同一个理由：
    // 它是"当前段"的量，段没了它就没有意义 —— 留着会让下一段继承上一段的结论。
    largest_gap_cells_ = 0;
    // `total_` / `observed_` / `skipped_` / `segments_` / `closed_span_ns_` 不清：
    // 证据要的是"整场会话发生过什么"，不是"此刻缓冲里有什么"。
    // 这正是 P13-C 那个 `audio.started` 被 `stop()` 清掉的同一个坑。
    has_last_ = false;
}

std::int64_t DriftSeries::media_span_ns() const noexcept {
    // 段内跨度恒 ≥ 0：媒体时刻倒退一律走 seek 分支（先 `clear()` 再起新段），
    // 所以同一段内 `segment_last_accepted_ns_ ≥ segment_first_media_ns_`。
    // 这里不夹到 0 —— 夹了就把"段内出现了倒退"这种真 bug 藏成一个合法的小数。
    const std::int64_t live =
        has_last_ ? (segment_last_accepted_ns_ - segment_first_media_ns_) : 0;
    return closed_span_ns_ + live;
}

DriftPoint DriftSeries::at(std::int32_t index) const noexcept {
    if (index < 0 || index >= size_) {
        return DriftPoint{};
    }
    // 未满时最旧点在 0；满了之后最旧点在 `write_index_`。
    const std::int32_t oldest = (size_ == kCapacity) ? write_index_ : 0;
    const std::int32_t slot = (oldest + index) % kCapacity;
    return points_[static_cast<std::size_t>(slot)];
}

DriftExtreme DriftSeries::max_point() const noexcept {
    // 极值在 `note()` 里 O(1) 累计维护，不在这里扫序列。
    //
    // 扫序列看着更"自然"，但它会在环形覆盖后与直方图分家：
    // 直方图是**累计**的，最大值若发生在已被覆盖的旧段，序列里已经找不到它，
    // 于是 `worst` 会小于 `max` —— 判据 F1 当场红，而且红得毫无道理
    // （两个字段都对，只是口径一个累计一个窗口）。
    // 累计维护让两边永远同口径。
    //
    // 这条不是推测：本卡的负向自测把实现换成扫序列跑过一次，
    // t07（覆盖后极值仍在）与 t08（`clear()` 后极值仍在）合计 6 条断言当场打红。
    DriftExtreme out = worst_;
    if (out.valid) {
        // `of` 用累计接受数（= PaceStats 的 `sampled_samples` = JSON 的 `samples`），
        // 不用窗口内 `size_` —— `index` 是全场序号，`of` 也得是全场的。
        out.of = static_cast<std::int32_t>(total_);
    }
    return out;
}

}  // namespace kivo::video::core::pace
