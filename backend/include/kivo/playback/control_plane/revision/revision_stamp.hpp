#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/callback_token.hpp"

namespace kivo::playback::control_plane::revision {

/// RevisionStamp is the sole authority for all async validation.
/// Must carry all 8 fields.
/// Validation order: session_id -> generation -> read_epoch -> seek_epoch ->
/// recovery_epoch -> stream_revision -> runtime_policy_revision -> callback_token -> reader instance validity.
struct RevisionStamp
{
    SessionId session_id{};
    SessionGeneration generation{};
    ReadEpoch read_epoch{};
    SeekEpoch seek_epoch{};
    RecoveryEpoch recovery_epoch{};
    StreamRevision stream_revision{};
    TimelineRevision timeline_revision{};
    P4RuntimePolicyRevision runtime_policy_revision{};
    CallbackToken callback_token{};

    constexpr RevisionStamp() noexcept = default;
};

} // namespace kivo::playback::control_plane::revision
