#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::backpressure {

/// ReadSchedulingDecision is the result of backpressure scheduling.
struct ReadSchedulingDecision
{
    bool should_schedule_read{false};
    primitives::Bytes requested_bytes{};
    primitives::U32 priority{0}; // 0=lowest
    primitives::U32 reason_code{0}; // StableReasonCode
};

/// TokenDebtPolicy controls urgent token debt allowance.
struct TokenDebtPolicy
{
    bool allow_urgent_token_debt{false};
    primitives::Count max_urgent_token_debt{};
};

/// BackpressureScheduler decides read scheduling with priority order.
/// Priority order:
/// Stop/Seek/Shutdown > MemoryPoolCritical > ResourceBudget >
/// TokenBucket > FastDrainUrgent > AdaptiveCooldown
class BackpressureScheduler
{
public:
    virtual ~BackpressureScheduler() = default;

    virtual ReadSchedulingDecision decide(
        const void* buffer_snapshot,
        const void* resource_budget,
        const void* pressure,
        const void* latest_hint,
        bool adaptive) = 0;
};

} // namespace kivo::playback::control_plane::backpressure
