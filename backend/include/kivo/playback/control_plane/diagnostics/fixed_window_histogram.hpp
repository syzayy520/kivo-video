#pragma once

#include <array>
#include <cstdint>
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::diagnostics {

/// FixedWindowHistogram is a no-heap sliding-window histogram.
/// No heap allocation in hot path. O(1) or near O(1) update.
template <primitives::U32 BucketCount, primitives::U32 SlotCount>
class FixedWindowHistogram
{
public:
    void update(primitives::U32 bucket_index, primitives::U64 value, primitives::MonotonicTimestamp now) noexcept
    {
        if (bucket_index >= BucketCount) { ++overflow_counts_; return; }
        maybe_rotate(now);
        buckets_[active_slot_][bucket_index].sum += value;
        ++buckets_[active_slot_][bucket_index].count;
    }

    primitives::U64 total_for_bucket(primitives::U32 bucket_index) const noexcept
    {
        if (bucket_index >= BucketCount) return 0;
        primitives::U64 total = 0;
        for (primitives::U32 s = 0; s < SlotCount; ++s)
            total += buckets_[s][bucket_index].sum;
        return total;
    }

    primitives::U32 overflow_count() const noexcept { return overflow_counts_; }

private:
    struct Bucket
    {
        primitives::U64 sum{0};
        primitives::U32 count{0};
    };

    void maybe_rotate(primitives::MonotonicTimestamp now) noexcept
    {
        if (now.ticks_ns >= active_slot_start_.ticks_ns + slot_duration_ms_.value)
        {
            active_slot_ = (active_slot_ + 1) % SlotCount;
            buckets_[active_slot_].fill(Bucket{});
            active_slot_start_ = now;
        }
    }

    std::array<std::array<Bucket, BucketCount>, SlotCount> buckets_{};
    primitives::U32 overflow_counts_{0};
    primitives::U32 active_slot_{0};
    primitives::Milliseconds slot_duration_ms_{1000};
    primitives::MonotonicTimestamp active_slot_start_{};
};

} // namespace kivo::playback::control_plane::diagnostics
