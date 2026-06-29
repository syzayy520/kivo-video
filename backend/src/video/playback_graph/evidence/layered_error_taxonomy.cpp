#include "video/playback_graph/evidence/layered_error_taxonomy.hpp"

#include <type_traits>

namespace kivo::video::playback_graph::runtime {

EvidenceSeverity severity_for_command_error(PlaybackGraphError error) noexcept {
    switch (error) {
        case PlaybackGraphError::None:
            return EvidenceSeverity::Info;
        case PlaybackGraphError::ClosedGraph:
        case PlaybackGraphError::InvalidState:
        case PlaybackGraphError::InvalidCommand:
            return EvidenceSeverity::CommandRejected;
        case PlaybackGraphError::RecoveryFailed:
        case PlaybackGraphError::SourceOpenFailed:
            return EvidenceSeverity::FatalGraph;
        default:
            return EvidenceSeverity::Recoverable;
    }
}

GraphEvidenceReason evidence_reason_for_late_discard() noexcept {
    return GraphEvidenceReason::LateCallbackDiscarded;
}

bool architecture_failure_user_facing(ArchitectureGuardFailure failure) noexcept {
    return failure == ArchitectureGuardFailure::None;
}

bool layered_error_models_are_distinct() noexcept {
    return !std::is_same_v<PlaybackGraphError, GraphEvidenceReason> &&
           !std::is_same_v<PlaybackGraphError, ArchitectureGuardFailure> &&
           !std::is_same_v<PlaybackGraphError, EvidenceSeverity> &&
           !std::is_same_v<GraphEvidenceReason, ArchitectureGuardFailure> &&
           !std::is_same_v<GraphEvidenceReason, EvidenceSeverity> &&
           !std::is_same_v<ArchitectureGuardFailure, EvidenceSeverity>;
}

}  // namespace kivo::video::playback_graph::runtime
