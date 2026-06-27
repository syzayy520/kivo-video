// cancel_mode.hpp — GATE-010: P5A cancel mode (4 values)
#pragma once

namespace kivo::video_plane::queue {

enum class CancelMode {
    Graceful,                               // wait for current item to finish
    Immediate,                              // cancel immediately
    Force,                                  // force cancel, may leave partial state
    Deferred                                // defer cancel until safe point
};

}  // namespace kivo::video_plane::queue
