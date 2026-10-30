#include "video/video_plane/core/pace/pace_histogram.hpp"

#include <algorithm>
#include <cstdlib>

namespace kivo::video::core::pace {

void SignedHistogram::add(std::int64_t value_ns) noexcept {
    if (count_ == 0) {
        min_ = value_ns;
        max_ = value_ns;
    } else {
        if (value_ns < min_) {
            min_ = value_ns;
        }
        if (value_ns > max_) {
            max_ = value_ns;
        }
    }
    ++count_;

    if (value_ns < kLowNs) {
        ++below_;
        return;
    }
    if (value_ns >= kHighNs) {
        ++above_;
        return;
    }
    const std::int64_t index = (value_ns - kLowNs) / kBucketWidthNs;
    ++bins_[static_cast<std::size_t>(index)];
}

void SignedHistogram::clear() noexcept {
    bins_.fill(0);
    below_ = 0;
    above_ = 0;
    count_ = 0;
    min_ = 0;
    max_ = 0;
}

SignedHistogram::Located SignedHistogram::locate(double q) const noexcept {
    Located out;

    // 秩取上整：100 个样本的 p99 是第 99 个，不是第 98.01 个。
    //
    // 这段与 `percentile_ns` 原来的内联版本**逐字一致** —— 抽出来只是为了让
    // 桶版和值版共用同一次判定，算式一个字都没动。
    std::int64_t rank = static_cast<std::int64_t>(q * static_cast<double>(count_));
    if (static_cast<double>(rank) < q * static_cast<double>(count_)) {
        ++rank;
    }
    if (rank < 1) {
        rank = 1;
    }

    if (rank <= below_) {
        out.bucket = PercentileBucket::kBelowRange;
        out.samples = below_;
        return out;
    }
    std::int64_t accumulated = below_;
    for (std::int32_t index = 0; index < kBucketCount; ++index) {
        const std::int64_t bucket_count = bins_[static_cast<std::size_t>(index)];
        accumulated += bucket_count;
        if (rank <= accumulated) {
            out.bucket = index;
            out.samples = bucket_count;
            return out;
        }
    }
    // 秩越过所有桶 → 落在上溢区。样本数给上溢计数，不是 0。
    out.bucket = kBucketCount;
    out.samples = above_;
    return out;
}

std::int64_t SignedHistogram::percentile_ns(double q) const noexcept {
    if (count_ == 0) {
        return 0;
    }
    if (q <= 0.0) {
        return min_;
    }
    if (q >= 1.0) {
        return max_;
    }

    // 结果一律钳进 [min, max]。
    // 不钳会出现"p99 = 0.125ms，max = 0.079ms"这种自相矛盾的证据 ——
    // 桶中心是 0.125 而真实极值是 0.079，读者只会认定数据有问题。
    const auto clamp_to_range = [this](std::int64_t value) {
        if (value < min_) {
            return min_;
        }
        if (value > max_) {
            return max_;
        }
        return value;
    };

    const Located at = locate(q);
    if (at.bucket == PercentileBucket::kBelowRange) {
        return min_;
    }
    if (at.bucket >= kBucketCount) {
        // 落在上溢桶
        return max_;
    }
    // 桶中心：桶 i 覆盖 [kLow + i*w, kLow + (i+1)*w)
    return clamp_to_range(bucket_center_ns(at.bucket));
}

PercentileBucket SignedHistogram::percentile_bucket(double q) const noexcept {
    PercentileBucket out;
    if (count_ == 0) {
        return out;  // index = kNoSamples, samples = 0, exact = false
    }

    // `q <= 0` / `q >= 1` 走的是 `percentile_ns` 的两个端点分支：
    // 那两处直接返回精确极值，一个桶都没碰 —— 所以 `exact = true`。
    // 这里必须跟着 `percentile_ns` 走，不能自己算一遍：
    // 若这边说"落在桶 0"而那边返回 min_，两格就打架了。
    if (q <= 0.0) {
        out.index = PercentileBucket::kBelowRange;
        out.samples = below_;
        out.exact = true;
        return out;
    }
    if (q >= 1.0) {
        out.index = kBucketCount;
        out.samples = above_;
        out.exact = true;
        return out;
    }

    const Located at = locate(q);
    out.index = at.bucket;
    out.samples = at.samples;
    out.exact = (at.bucket < 0) || (at.bucket >= kBucketCount);
    return out;
}

SignedHistogram::PercentileRange SignedHistogram::percentile_range(double q) const noexcept {
    PercentileRange out;
    if (count_ == 0) {
        return out;
    }

    // 从 `percentile_ns` 取那个数本身，而不是重算一遍桶中心 ——
    // 重算就得复制 `clamp_to_range`，那是第三个真相的入口。
    const std::int64_t value = percentile_ns(q);
    out.low_ns = value;
    out.high_ns = value;

    const PercentileBucket at = percentile_bucket(q);
    if (!at.in_bucket()) {
        // 溢出区或 `q` 落在端点：`percentile_ns` 返回的是精确极值。
        // 范围退化成那个点 —— 这是**更准**，不是"范围没算出来"。
        return out;
    }

    // 与 `[min_, max_]` 求交：
    //   · 下界取大 —— 真值不可能低于最小样本；
    //   · 上界取 `max_ + 1`（开界）—— 真值不可能大于最大样本。
    // 两边各取各的极值，取反了就会得到一个不含 `value` 的空区间，
    // 而空区间在 JSON 里长得像"范围很窄、很准"。
    out.low_ns = std::max(bucket_low_ns(at.index), min_);
    out.high_ns = std::min(bucket_high_ns(at.index), max_ + 1);
    out.bounded = out.low_ns < out.high_ns;
    return out;
}

std::int64_t SignedHistogram::count_abs_beyond(std::int64_t threshold_ns) const noexcept {
    if (count_ == 0 || threshold_ns < 0) {
        return 0;
    }

    std::int64_t total = 0;
    // 溢出桶：下溢桶全部 <= kLowNs，上溢桶全部 >= kHighNs。
    // 只要阈值比它们更靠内，就整桶算超出。
    if (below_ > 0 && -kLowNs > threshold_ns) {
        total += below_;
    }
    if (above_ > 0 && kHighNs > threshold_ns) {
        total += above_;
    }

    for (std::int32_t index = 0; index < kBucketCount; ++index) {
        const std::int64_t bucket_count = bins_[static_cast<std::size_t>(index)];
        if (bucket_count == 0) {
            continue;
        }
        const std::int64_t center_ns = bucket_center_ns(index);
        if (std::llabs(center_ns) > threshold_ns) {
            total += bucket_count;
        }
    }
    return total;
}

}  // namespace kivo::video::core::pace
