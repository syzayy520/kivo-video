// queue_lifecycle_state.hpp — GATE-010: P5A queue lifecycle state (7 values)
#pragma once

namespace kivo::video_plane::queue {

enum class QueueLifecycleState {
    Accepting,                              // queue is accepting new items
    Processing,                             // queue is processing items
    Cancelling,                             // cancellation in progress
    Draining,                               // draining remaining items
    Drained,                                // all items drained
    Rejected,                               // item rejected
    Failed                                  // queue failed
};

}  // namespace kivo::video_plane::queue
