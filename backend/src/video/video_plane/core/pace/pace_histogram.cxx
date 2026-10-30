#include "video/video_plane/core/pace/pace_histogram.hpp"

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

    // 秩取上整：100 个样本的 p99 是第 99 个，不是第 98.01 个。
    std::int64_t rank = static_cast<std::int64_t>(q * static_cast<double>(count_));
    if (static_cast<double>(rank) < q * static_cast<double>(count_)) {
        ++rank;
    }
    if (rank < 1) {
        rank = 1;
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

    if (rank <= below_) {
        return min_;
    }
    std::int64_t accumulated = below_;
    for (std::int32_t index = 0; index < kBucketCount; ++index) {
        accumulated += bins_[static_cast<std::size_t>(index)];
        if (rank <= accumulated) {
            // 桶中心：桶 i 覆盖 [kLow + i*w, kLow + (i+1)*w)
            return clamp_to_range(bucket_center_ns(index));
        }
    }
    // 落在上溢桶
    return max_;
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
