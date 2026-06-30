#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

struct SubtitleCueSnapshot {
    bool visible{false};
};

struct SubtitleSnapshot {
    std::uint64_t selected_track_id{0};
    bool enabled{false};
    std::int64_t delay_ms{0};
    bool frame_available{false};
    SubtitleCueSnapshot primary{};
    SubtitleCueSnapshot secondary{};
    bool valid{false};
};

struct SubtitleDelayRequest {
    std::int64_t delay_ms{0};
};

}  // namespace kivo::video::playback_graph