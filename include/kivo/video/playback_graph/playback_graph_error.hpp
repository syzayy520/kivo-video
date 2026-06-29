#pragma once

namespace kivo::video::playback_graph {

enum class PlaybackGraphError {
    None,
    InvalidState,
    InvalidCommand,
    InvalidSeekTarget,
    ClosedGraph,
    SourceOpenFailed,
    SourceReadFailed,
    SourceSeekFailed,
    CapabilityNegotiationFailed,
    ProtectedContentUnsupported,
    AttachmentCompletionTimeout,
    SeekTimeout,
    DrainTimeout,
    CloseTimeout,
    RecoveryFailed,
    OutOfMemory,
    CommandRegistryFull,
    P4CommandCreateFailed,
    SidecarInstallFailed,
    CommandQueueOverflow,
    PlaybackRevisionMismatch,
    GraphSnapshotMismatch,
    GenerationStampMismatch,
    PolicyRejectedSafetyCritical,
    PolicyAdjustedNonCritical,
    PolicyUnsupportedByCapability
};

enum class CommandTerminalStatus {
    None,
    Completed,
    Failed,
    Cancelled,
    Superseded,
    Expired,
    NoOp,
    Rejected
};

enum class GraphEvidenceReason {
    None,
    LateCallbackDiscarded,
    LateRecycleAckDiscarded,
    DuplicateRecycleAckDetected,
    StampReadUnstable,
    CriticalEventOverflow,
    PerformanceBudgetExceeded,
    PolicyAdjustedNonCritical,
    MicroAdjustCapabilityChanged,
    ObserverCallbackFailed
};

enum class ArchitectureGuardFailure {
    None,
    PublicHeaderPuristFailed,
    ForbiddenDependencyFound,
    ContractHeaderMismatch,
    ContractBehaviorMismatch,
    HotPathAllocationViolation,
    HotPathLoggingViolation,
    HotPathBlockingViolation
};

enum class EvidenceSeverity {
    Info,
    Warning,
    Recoverable,
    CommandRejected,
    DiagnosticsOnly,
    FatalGraph,
    ArchitectureOnly,
    TestOnly
};

}  // namespace kivo::video::playback_graph
