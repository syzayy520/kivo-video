#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

struct SubtitleCueSnapshot {
    std::uint64_t cue_id{0};
    std::int64_t start_ms{0};
    std::int64_t end_ms{0};
    char primary_text[256]{};
    char secondary_text[256]{};
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