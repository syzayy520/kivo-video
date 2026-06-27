#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::reader {

/// LowSpeedPolicy defines low speed detection thresholds.
struct LowSpeedPolicy
{
    bool enable_low_speed_detection{true};
    primitives::BytesPerSecond min_bytes_per_second{};
    primitives::Milliseconds observe_window_ms{};
    primitives::Milliseconds violation_grace_ms{};
    primitives::U32 max_consecutive_violations{0};
    primitives::Milliseconds hysteresis_recover_ms{};
};

} // namespace kivo::playback::control_plane::reader
