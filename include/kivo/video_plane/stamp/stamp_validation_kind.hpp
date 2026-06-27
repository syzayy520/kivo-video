#pragma once

namespace kivo::video_plane::stamp {

enum class StampValidationKind {
    BootstrapCheck,           // initial pipeline setup
    IncrementalAdvance,       // normal forward progress
    SeekReposition,           // user or recovery seek
    PipelineEpochChange,      // P4 session changed
    RecoveryRejoin,           // after device lost / rebuild
    InputGap,                 // discontinuity in input
    QueueStarvationRecheck    // queue starved, recheck stamp validity
};

}  // namespace kivo::video_plane::stamp
