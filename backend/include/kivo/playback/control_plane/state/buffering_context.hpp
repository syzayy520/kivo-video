#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/state/session_state.hpp"

namespace kivo::playback::control_plane::state {

/// StartBufferReason enumerates buffer start reasons.
enum class StartBufferReason
{
    InitialStart,
    SeekWarmup,
    RecoveryResume,
    PausedSeekWarmup,
    RateRelock
};

/// BufferingContext tracks state before buffering for pause override.
/// Hysteresis does NOT hold pending state. StateMachine holds BufferingContext.
/// Pause after Buffering must override pending Playing.
/// Stop/Seek/Recovery clears BufferingContext.
struct BufferingContext
{
    SessionState state_before_buffering{SessionState::Idle};
    revision::CommandSequence state_command_sequence{};
    bool user_pause_after_entered{false};
};

/// StartBufferDecision is the result of start buffer evaluation.
/// Hot path reason_code uses StableReasonCode, NOT std::string.
struct StartBufferDecision
{
    bool satisfied{false};
    primitives::Bytes required_bytes{};
    primitives::Milliseconds required_ms{};
    // TimeMappingConfidence referenced via timeline header in full implementation
    primitives::U32 reason_code{0}; // StableReasonCode value
};

} // namespace kivo::playback::control_plane::state
