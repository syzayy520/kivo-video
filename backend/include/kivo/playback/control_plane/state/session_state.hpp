#pragma once

namespace kivo::playback::control_plane::state {

enum class SessionState
{
    Idle,
    Initializing,
    Ready,
    Playing,
    Paused,
    Seeking,
    Buffering,
    ErrorRecovery,
    Stopping,
    Terminated
};

constexpr bool is_valid_transition(SessionState from, SessionState to) noexcept
{
    if (from == to) return true;
    switch (from)
    {
    case SessionState::Idle:
        return to == SessionState::Initializing || to == SessionState::Terminated;
    case SessionState::Initializing:
        return to == SessionState::Ready || to == SessionState::Playing ||
               to == SessionState::Buffering || to == SessionState::ErrorRecovery ||
               to == SessionState::Stopping || to == SessionState::Terminated;
    case SessionState::Ready:
        return to == SessionState::Playing || to == SessionState::Seeking ||
               to == SessionState::Stopping || to == SessionState::Terminated;
    case SessionState::Playing:
        return to == SessionState::Paused || to == SessionState::Seeking ||
               to == SessionState::Buffering || to == SessionState::ErrorRecovery ||
               to == SessionState::Stopping || to == SessionState::Terminated;
    case SessionState::Paused:
        return to == SessionState::Playing || to == SessionState::Seeking ||
               to == SessionState::Stopping || to == SessionState::Terminated;
    case SessionState::Seeking:
        return to == SessionState::Ready || to == SessionState::Playing ||
               to == SessionState::Paused || to == SessionState::Buffering ||
               to == SessionState::ErrorRecovery || to == SessionState::Stopping ||
               to == SessionState::Terminated;
    case SessionState::Buffering:
        return to == SessionState::Playing || to == SessionState::Paused ||
               to == SessionState::Seeking || to == SessionState::ErrorRecovery ||
               to == SessionState::Stopping || to == SessionState::Terminated;
    case SessionState::ErrorRecovery:
        return to == SessionState::Ready || to == SessionState::Playing ||
               to == SessionState::Paused || to == SessionState::Buffering ||
               to == SessionState::Stopping || to == SessionState::Terminated;
    case SessionState::Stopping:
        return to == SessionState::Terminated;
    case SessionState::Terminated:
        return false;
    }
    return false;
}

} // namespace kivo::playback::control_plane::state
