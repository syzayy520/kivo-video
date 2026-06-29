#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

enum class SeekKind {
    Absolute,
    Relative,
    LiveEdge,
    ReprimeCurrent
};

struct SeekRequest {
    SeekKind kind{SeekKind::Absolute};
    std::int64_t target_timeline_ms{0};
    bool start_after_seek{false};
};

}  // namespace kivo::video::playback_graph
