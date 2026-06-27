// subtitle_composition_snapshot.hpp — GATE-023: P5A subtitle composition snapshot
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::output_boundary {

struct SubtitleCompositionSnapshot {
    uint64_t snapshot_id{0};                // unique snapshot identifier
    uint64_t media_timestamp{0};            // media timestamp of snapshot
    std::vector<std::string> active_subtitle_ids; // active subtitle track ids
    bool is_empty{true};                    // no subtitles active
    bool operator==(SubtitleCompositionSnapshot const&) const = default;
};

}  // namespace kivo::video_plane::output_boundary
