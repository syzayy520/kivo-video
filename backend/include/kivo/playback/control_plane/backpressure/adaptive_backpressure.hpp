#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/rate/playback_rate_ratio.hpp"
#include "kivo/playback/control_plane/primitives/ratio.hpp"

namespace kivo::playback::control_plane::backpressure {

/// AdaptiveBackpressureConfig defines grow/shrink thresholds and sampling.
/// All grow/shrink uses Ratio, NOT double.
struct AdaptiveBackpressureConfig
{
    primitives::Milliseconds sample_interval_ms{};
    primitives::Milliseconds sliding_window_ms{};
    primitives::Ratio grow_factor{};
    primitives::Ratio shrink_factor{};
    primitives::U32 would_block_grow_threshold{0};
    primitives::U32 buffering_churn_threshold{0};
    primitives::U32 mapping_error_threshold{0};
    primitives::Ratio high_memory_pressure_shrink_factor{};
    primitives::Ratio starvation_priority_boost_factor{};
    primitives::Milliseconds min_window_ms{};
    primitives::Bytes min_window_bytes{};
};

/// AdaptiveBackpressureSnapshot captures observed fill/drain and churn metrics.
struct AdaptiveBackpressureSnapshot
{
    primitives::BytesPerSecond observed_fill_bytes_per_second{};
    primitives::BytesPerSecond observed_drain_bytes_per_second{};
    primitives::Milliseconds average_recovery_ms{};
    primitives::U32 would_block_count_recent{0};
    primitives::U32 buffering_churn_count_recent{0};
    primitives::U32 mapping_error_count_recent{0};
    bool downstream_starvation_detected{false};
    bool byte_time_mapping_unstable{false};
    bool current_window_too_small{false};
    bool current_window_too_large{false};
    bool urgent_read_demand{false};
};

/// AdaptiveBackpressureGuard enforces window upper bounds.
/// Window never exceeds guard.
/// Urgent read cannot break ResourceBudget or MemoryPoolCritical.
struct AdaptiveBackpressureGuard
{
    primitives::Milliseconds window_upper_bound_ms{};
    primitives::Bytes window_upper_bound_bytes{};
    primitives::Milliseconds interleave_max_window_ms{};
    primitives::Bytes interleave_max_window_bytes{};
    primitives::Milliseconds live_max_window_ms{};
    primitives::Bytes resource_max_bytes{};
};

} // namespace kivo::playback::control_plane::backpressure
