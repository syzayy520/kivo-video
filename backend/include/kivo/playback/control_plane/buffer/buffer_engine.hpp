#pragma once

#include "kivo/playback/control_plane/buffer/buffer_item.hpp"
#include "kivo/playback/control_plane/buffer/buffer_snapshot.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::buffer {

/// BufferPushStatus enumerates buffer push outcomes.
/// Legal matrix:
/// - Accepted: writes buffer yes, diagnostics optional, may block no.
/// - WouldBlockFull: writes buffer no, diagnostics yes, may block yes (cancellable).
/// - RejectedStaleStamp: writes buffer no, diagnostics stale counter, may block no.
/// - RejectedFenced: writes buffer no, diagnostics fence counter, may block no.
/// - Cancelled: writes buffer no, diagnostics optional, may block no.
enum class BufferPushStatus
{
    Accepted,
    WouldBlockFull,
    RejectedStaleStamp,
    RejectedFenced,
    Cancelled
};

/// BufferPushResult is the result of a buffer push.
struct BufferPushResult
{
    BufferPushStatus status{BufferPushStatus::Accepted};
};

/// BufferWakeReason enumerates reasons to wake buffer waiters.
enum class BufferWakeReason
{
    Stop,
    Seek,
    Flush,
    IdleRelease,
    Shutdown,
    ResourceBudgetChanged
};

/// BufferPushPolicy controls push behavior.
struct BufferPushPolicy
{
    bool allow_blocking{false};
    primitives::U32 stale_counter{0};
    primitives::U32 fence_counter{0};
};

/// BufferEngine maintains one ordered item stream with push/wake/snapshot.
class BufferEngine
{
public:
    virtual ~BufferEngine() = default;

    virtual BufferPushResult push(const BufferItem& item, const BufferPushPolicy& policy) = 0;
    virtual void wake_all_waiters(BufferWakeReason reason) = 0;
    virtual BufferSnapshot snapshot() const = 0;
};

} // namespace kivo::playback::control_plane::buffer
