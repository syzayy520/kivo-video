#pragma once

#include <optional>
#include "kivo/playback/control_plane/recovery/playback_failure.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/state/session_state.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::recovery {

/// RecoveryDecisionKind enumerates recovery decision kinds.
enum class RecoveryDecisionKind
{
    RetryRead,
    ReopenReader,
    ReplaceReader,
    FallbackToAlternativeCandidate,
    IncrementStreamRevision,
    HardTerminateToUpperLayer,
    NoAction
};

/// RecoveryDecisionResult is the result of a recovery decision.
struct RecoveryDecisionResult
{
    RecoveryDecisionKind kind{RecoveryDecisionKind::NoAction};
    revision::RecoveryEpoch recovery_epoch{};
    state::SessionState state_before_recovery{state::SessionState::Idle};
    state::SessionState target_state{state::SessionState::Idle};
    std::optional<primitives::U64> fallback{};
    primitives::Milliseconds backoff_delay_ms{};
    bool requires_reader_replace{false};
    bool requires_stream_revision_increment{false};
    bool requires_buffer_flush{false};
    bool used_destructive_cancel{false};
    primitives::U32 stable_reason_code{0};
};

/// RecoveryContext tracks an active recovery process.
struct RecoveryContext
{
    revision::RecoveryEpoch recovery_epoch{};
    PlaybackFailure root_failure{};
    state::SessionState state_before_recovery{state::SessionState::Idle};
    RecoveryDecisionResult decision{};
    primitives::MonotonicTimestamp started_at{};
    revision::RevisionStamp associated_stamp{};
};

} // namespace kivo::playback::control_plane::recovery
