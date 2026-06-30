#pragma once

#include <cstddef>

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_state.hpp"

namespace kivo::video::playback_graph::runtime {

enum class GraphCommandKind {
    Open,
    Start,
    Pause,
    Resume,
    Seek,
    SwitchAudioTrack,
    SwitchVideoTrack,
    SwitchAvTrackSet,
    Close,
    Stop,
    SwitchSubtitleTrack,
    DisableSubtitle,
    SetSubtitleDelay,
    SetAudioOutputPolicy,
    SetPlaybackSettingsPolicy,
    InjectSystemEvent,
    RecoveryReplay,
    CycleSubtitleTrack,
    CycleAudioTrack
};

enum class CommandConflictDecision {
    Accept,
    PendingTransportIntent,
    RejectInvalidState,
    RejectClosedGraph
};

struct CommandConflictResult {
    CommandConflictDecision decision{CommandConflictDecision::RejectInvalidState};
    PlaybackGraphError error{PlaybackGraphError::InvalidState};
    bool creates_pending_transport_intent{false};
};

[[nodiscard]] CommandConflictResult resolve_command_conflict(
    GraphCommandKind command,
    PlaybackGraphState state,
    bool has_pending_transport_intent) noexcept;
[[nodiscard]] const char* command_kind_name(GraphCommandKind command) noexcept;
[[nodiscard]] const char* command_conflict_decision_name(CommandConflictDecision decision) noexcept;
[[nodiscard]] std::size_t command_conflict_matrix_rule_count() noexcept;

}  // namespace kivo::video::playback_graph::runtime
