#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

struct BufferedTimeRange {
    std::int64_t start_ms{0};
    std::int64_t end_ms{0};
};

struct PlaybackTimelineSnapshot {
    std::int64_t position_ms{0};
    std::int64_t duration_ms{0};
    BufferedTimeRange buffered_range{};
    bool seek_in_progress{false};
    std::int64_t last_seek_target_ms{0};
    bool valid{false};
};

}  // namespace kivo::video::playback_graph