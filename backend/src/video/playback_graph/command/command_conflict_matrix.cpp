#include "video/playback_graph/command/command_conflict_matrix.hpp"

#include <array>

namespace kivo::video::playback_graph::runtime {
namespace {

struct MatrixRule {
    GraphCommandKind command;
    PlaybackGraphState state;
    CommandConflictDecision decision;
};

constexpr std::array<MatrixRule, 11> kRules{{
    {GraphCommandKind::Open, PlaybackGraphState::NotCreated, CommandConflictDecision::Accept},
    {GraphCommandKind::Open, PlaybackGraphState::BuildFailed, CommandConflictDecision::Accept},
    {GraphCommandKind::Start, PlaybackGraphState::Ready, CommandConflictDecision::Accept},
    {GraphCommandKind::Pause, PlaybackGraphState::Starting, CommandConflictDecision::PendingTransportIntent},
    {GraphCommandKind::Pause, PlaybackGraphState::Playing, CommandConflictDecision::Accept},
    {GraphCommandKind::Resume, PlaybackGraphState::Paused, CommandConflictDecision::Accept},
    {GraphCommandKind::Seek, PlaybackGraphState::Ready, CommandConflictDecision::Accept},
    {GraphCommandKind::Seek, PlaybackGraphState::Playing, CommandConflictDecision::Accept},
    {GraphCommandKind::Seek, PlaybackGraphState::Paused, CommandConflictDecision::Accept},
    {GraphCommandKind::SwitchAudioTrack, PlaybackGraphState::Playing, CommandConflictDecision::Accept},
    {GraphCommandKind::SwitchVideoTrack, PlaybackGraphState::Playing, CommandConflictDecision::Accept},
}};

[[nodiscard]] bool is_closed_state(PlaybackGraphState state) noexcept {
    return state == PlaybackGraphState::Closing || state == PlaybackGraphState::Closed;
}

[[nodiscard]] CommandConflictResult from_decision(CommandConflictDecision decision) noexcept {
    switch (decision) {
        case CommandConflictDecision::Accept:
            return CommandConflictResult{decision, PlaybackGraphError::None, false};
        case CommandConflictDecision::PendingTransportIntent:
            return CommandConflictResult{decision, PlaybackGraphError::None, true};
        case CommandConflictDecision::RejectClosedGraph:
            return CommandConflictResult{decision, PlaybackGraphError::ClosedGraph, false};
        case CommandConflictDecision::RejectInvalidState:
            return CommandConflictResult{decision, PlaybackGraphError::InvalidState, false};
    }
    return CommandConflictResult{};
}

}  // namespace

CommandConflictResult resolve_command_conflict(GraphCommandKind command,
                                               PlaybackGraphState state,
                                               bool has_pending_transport_intent) noexcept {
    if (command == GraphCommandKind::Close) {
        return is_closed_state(state)
                   ? from_decision(CommandConflictDecision::RejectClosedGraph)
                   : from_decision(CommandConflictDecision::Accept);
    }
    if (is_closed_state(state)) {
        return from_decision(CommandConflictDecision::RejectClosedGraph);
    }
    if (has_pending_transport_intent &&
        (command == GraphCommandKind::Pause || command == GraphCommandKind::Resume)) {
        return from_decision(CommandConflictDecision::PendingTransportIntent);
    }
    if (command == GraphCommandKind::SwitchAvTrackSet && state == PlaybackGraphState::Playing) {
        return from_decision(CommandConflictDecision::Accept);
    }
    if (command == GraphCommandKind::InjectSystemEvent) {
        return from_decision(CommandConflictDecision::Accept);
    }
    if (command == GraphCommandKind::RecoveryReplay && state == PlaybackGraphState::Recovering) {
        return from_decision(CommandConflictDecision::Accept);
    }

    for (const auto& rule : kRules) {
        if (rule.command == command && rule.state == state) {
            return from_decision(rule.decision);
        }
    }

    return from_decision(CommandConflictDecision::RejectInvalidState);
}

const char* command_kind_name(GraphCommandKind command) noexcept {
    switch (command) {
        case GraphCommandKind::Open:
            return "Open";
        case GraphCommandKind::Start:
            return "Start";
        case GraphCommandKind::Pause:
            return "Pause";
        case GraphCommandKind::Resume:
            return "Resume";
        case GraphCommandKind::Seek:
            return "Seek";
        case GraphCommandKind::SwitchAudioTrack:
            return "SwitchAudioTrack";
        case GraphCommandKind::SwitchVideoTrack:
            return "SwitchVideoTrack";
        case GraphCommandKind::SwitchAvTrackSet:
            return "SwitchAvTrackSet";
        case GraphCommandKind::Close:
            return "Close";
        case GraphCommandKind::InjectSystemEvent:
            return "InjectSystemEvent";
        case GraphCommandKind::RecoveryReplay:
            return "RecoveryReplay";
    }
    return "Unknown";
}

const char* command_conflict_decision_name(CommandConflictDecision decision) noexcept {
    switch (decision) {
        case CommandConflictDecision::Accept:
            return "Accept";
        case CommandConflictDecision::PendingTransportIntent:
            return "PendingTransportIntent";
        case CommandConflictDecision::RejectInvalidState:
            return "RejectInvalidState";
        case CommandConflictDecision::RejectClosedGraph:
            return "RejectClosedGraph";
    }
    return "Unknown";
}

std::size_t command_conflict_matrix_rule_count() noexcept {
    return kRules.size();
}

}  // namespace kivo::video::playback_graph::runtime
