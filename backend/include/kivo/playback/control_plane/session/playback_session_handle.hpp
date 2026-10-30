#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/command/playback_command.hpp"
#include "kivo/playback/control_plane/command/command_submit_status.hpp"
#include "kivo/playback/control_plane/control/seek_target.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::session {

/// EventSubscriptionMask is a bitmask of event categories the caller wants.
using EventSubscriptionMask = primitives::U64;

/// PlaybackSnapshotTag marks the kind of snapshot requested.
enum class PlaybackSnapshotTag
{
    Full,
    Timeline,
    Diagnostics
};

/// PlaybackSessionHandle is the external, opaque handle to a P4 playback
/// session. P8 holds this handle; it must NOT directly construct
/// PlaybackCommand (internal id/sequence/generation are P4-only).
///
/// Concrete implementation lives in the P4 runtime layer. This header only
/// defines the contract surface.
class PlaybackSessionHandle
{
public:
    virtual ~PlaybackSessionHandle() = default;

    virtual revision::SessionId id() const noexcept = 0;

    /// Enqueues a command request. A callback invoking this must only
    /// enqueue, NOT synchronously execute.
    virtual command::CommandSubmitStatus submit_command_request(
        const command::PlaybackCommandRequest& request) = 0;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void seek(const control::SeekTarget& target) = 0;
    virtual void stop() = 0;
    virtual void flush(command::FlushScope scope) = 0;
    virtual void set_playback_rate(primitives::Milliseconds rate) = 0;
    virtual void request_user_track_switch(
        const command::UserTrackSwitchRequest& request) = 0;

    virtual void snapshot() = 0;
    virtual void timeline_snapshot() = 0;
    virtual void diagnostics_snapshot() = 0;
    virtual void stream_pipeline() = 0;

    virtual void subscribe(EventSubscriptionMask mask,
                           void (*callback)(void*, EventSubscriptionMask),
                           void* user_data) = 0;

    virtual void request_shutdown(primitives::U64 policy) = 0;
};

} // namespace kivo::playback::control_plane::session
