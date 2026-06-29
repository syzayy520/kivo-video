#pragma once

namespace kivo::video::playback_graph {

enum class PlaybackGraphState {
    NotCreated,
    Created,
    Building,
    BuildFailed,
    Ready,
    Starting,
    Buffering,
    Playing,
    Rebuffering,
    Stalled,
    Pausing,
    Paused,
    Resuming,
    Seeking,
    Flushing,
    TrackSwitching,
    Draining,
    EosReached,
    SystemSuspending,
    SystemSuspended,
    SystemResuming,
    Recovering,
    Failed,
    Closing,
    Closed
};

enum class BuildFailureKind {
    None,
    NoResource,
    PartialResource
};

}  // namespace kivo::video::playback_graph
