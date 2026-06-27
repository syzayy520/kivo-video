#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::control {

/// FlushScopeKind enumerates flush scope kinds with revision semantics.
/// | FlushScope       | cancel read | clear buffer | read_epoch++ | stream_revision++ | timeline_revision++ |
/// | BufferOnly       | no          | yes          | no           | no                | maybe               |
/// | ReaderInFlight   | yes         | maybe        | yes          | no                | no                  |
/// | StreamRevision   | yes         | yes          | yes          | yes               | yes                 |
/// | TimelinePending  | no          | no           | no           | no                | yes                 |
/// | FullSession      | yes         | yes          | yes          | yes               | yes                 |
enum class FlushScopeKind
{
    BufferOnly,
    ReaderInFlight,
    StreamRevision,
    TimelinePending,
    FullSession
};

/// FlushScopeSemantics captures the revision-table behavior for a flush scope.
struct FlushScopeSemantics
{
    bool cancel_read{false};
    bool clear_buffer{false};
    bool increment_read_epoch{false};
    bool increment_stream_revision{false};
    bool maybe_increment_timeline_revision{false};
};

/// Returns the semantics for a flush scope kind.
inline FlushScopeSemantics flush_scope_semantics(FlushScopeKind kind) noexcept
{
    switch (kind)
    {
    case FlushScopeKind::BufferOnly:
        return {false, true, false, false, true};
    case FlushScopeKind::ReaderInFlight:
        return {true, false, true, false, false}; // clear_buffer=maybe -> modeled as false
    case FlushScopeKind::StreamRevision:
        return {true, true, true, true, true};
    case FlushScopeKind::TimelinePending:
        return {false, false, false, false, true};
    case FlushScopeKind::FullSession:
        return {true, true, true, true, true};
    }
    return {};
}

/// FlushScope pairs a kind with causative epochs.
/// Flush must carry SeekEpoch/RecoveryEpoch when caused by seek or recovery.
struct FlushScope
{
    FlushScopeKind kind{FlushScopeKind::BufferOnly};
    revision::SeekEpoch seek_epoch{};
    revision::RecoveryEpoch recovery_epoch{};
};

} // namespace kivo::playback::control_plane::control
