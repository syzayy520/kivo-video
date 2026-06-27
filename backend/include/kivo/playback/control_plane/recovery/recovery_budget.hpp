#pragma once

#include "kivo/playback/control_plane/recovery/playback_failure_kind.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::recovery {

/// RecoveryBudget defines max limits for recovery actions.
/// RecoveryBudget must NOT allow unbounded retry/fallback.
struct RecoveryBudget
{
    primitives::Count max_retry_count{};
    primitives::Count max_reopen_count{};
    primitives::Count max_reader_replace_count{};
    primitives::Count max_fallback_count{};
    primitives::Milliseconds max_total_recovery_ms{};
    primitives::Milliseconds retry_backoff_initial_ms{};
    primitives::Milliseconds retry_backoff_max_ms{};
    primitives::Milliseconds graceful_cancel_timeout_ms{};
    primitives::Milliseconds destructive_cancel_timeout_ms{};
    bool allow_fallback{false};
    bool allow_destructive_cancel{false};
};

/// RecoveryBudgetCounter tracks recovery action counts per failure kind.
struct RecoveryBudgetCounter
{
    PlaybackFailureKind failure_kind{PlaybackFailureKind::Unknown};
    primitives::U32 retry_count{0};
    primitives::U32 reopen_count{0};
    primitives::U32 replace_count{0};
    primitives::U32 fallback_count{0};
};

} // namespace kivo::playback::control_plane::recovery
