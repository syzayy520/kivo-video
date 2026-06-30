#pragma once

namespace kivo::video::playback_graph {

enum class DiagnosticsInvalidReason {
    None,
    SessionNotReady,
    GraphClosed
};

}  // namespace kivo::video::playback_graph