#pragma once

namespace kivo::playback::control_plane::reader {

/// ReaderInstanceState enumerates reader instance lifecycle states.
/// Sole authority: reader/reader_instance_state.hpp
enum class ReaderInstanceState
{
    Registered,
    Opening,
    Open,
    Reading,
    Cancelling,
    Closing,
    Closed,
    Invalidated,
    Quarantined,
    Destroyed
};

/// Returns true if state is usable for operations.
constexpr bool is_usable_state(ReaderInstanceState state) noexcept
{
    return state == ReaderInstanceState::Registered
        || state == ReaderInstanceState::Opening
        || state == ReaderInstanceState::Open
        || state == ReaderInstanceState::Reading;
}

/// Returns true if state is permanently unusable.
constexpr bool is_permanently_unusable(ReaderInstanceState state) noexcept
{
    return state == ReaderInstanceState::Invalidated
        || state == ReaderInstanceState::Quarantined
        || state == ReaderInstanceState::Destroyed;
}

} // namespace kivo::playback::control_plane::reader
