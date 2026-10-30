#pragma once

#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::revision {

/// CancellationScope enumerates cancellation scopes.
enum class CancellationScope
{
    CurrentReadOnly,
    ReaderInFlight,
    ReaderAndBuffer,
    StreamRevision,
    Session,
    Shutdown,
    DestructiveReaderAbort
};

/// CancellationToken must NOT be externally constructible.
/// Only P4 cancellation factory can generate.
/// Old token cannot cancel new read.
struct CancellationToken
{
    SessionId session_id{};
    SessionGeneration generation{};
    ReadEpoch read_epoch{};
    primitives::U64 nonce{0};
};

/// ReaderCancelRequest pairs stamp with cancellation.
/// Seek/Stop/Shutdown can wake infinite deadline read.
/// DestructiveReaderAbort must enter diagnostics.
struct ReaderCancelRequest
{
    RevisionStamp stamp{};
    CancellationToken cancellation{};
    CancellationScope scope{CancellationScope::CurrentReadOnly};
};

} // namespace kivo::playback::control_plane::revision
