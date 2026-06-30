#pragma once

#include <cstddef>
#include <cstdint>

namespace kivo::video::playback_graph {

struct TrackInventorySnapshot {
    static constexpr std::size_t kMaxTracks = 8;

    std::uint64_t audio_track_ids[kMaxTracks]{};
    std::size_t audio_track_count{0};
    std::uint64_t selected_audio_track_id{0};

    std::uint64_t subtitle_track_ids[kMaxTracks]{};
    std::size_t subtitle_track_count{0};
    std::uint64_t selected_subtitle_track_id{0};

    std::uint64_t video_track_ids[kMaxTracks]{};
    std::size_t video_track_count{0};
    std::uint64_t selected_video_track_id{0};

    bool valid{false};
};

}  // namespace kivo::video::playback_graph