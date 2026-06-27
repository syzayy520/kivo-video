#pragma once

namespace kivo::playback::control_plane::command {

/// CommandSubmitStatus enumerates command submission outcomes.
/// Stop: first Stop in non-Stopping/Terminated returns Accepted.
/// Stopping: subsequent Stop returns AcceptedAlreadyInProgress.
/// Terminated: returns RejectedTerminated.
/// Stop is NOT coalesced, NOT dropped.
enum class CommandSubmitStatus
{
    Accepted,
    AcceptedAlreadyInProgress,
    Coalesced,
    DuplicateIgnored,
    RejectedTerminated,
    RejectedInvalidState,
    RejectedQueueFull,
    RejectedResourceBudget,
    RejectedPolicy,
    RejectedInvalidContext
};

} // namespace kivo::playback::control_plane::command
