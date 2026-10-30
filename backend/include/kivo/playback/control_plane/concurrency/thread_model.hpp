#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"

namespace kivo::playback::control_plane::concurrency {

/// ThreadKind enumerates P4 logical threads.
/// ControlThread is sole writer of SessionState/RevisionBarrier/CommandQueue/TimelineState.
/// ReaderWorkerThread may block on reader I/O but must NOT modify SessionState.
/// P5PullThread calls IMediaStreamPipeline::pull as single logical consumer.
/// P6ClockObservationPath must be realtime-safe for AudioOutput.
/// EventDeliveryThread must NOT hold internal locks when calling user callbacks.
/// DiagnosticsThread reads immutable snapshots only.
enum class ThreadKind
{
    ControlThread,
    ReaderWorkerThread,
    P5PullThread,
    P6ClockObservationPath,
    EventDeliveryThread,
    DiagnosticsThread,
    TestHarnessThread
};

} // namespace kivo::playback::control_plane::concurrency
