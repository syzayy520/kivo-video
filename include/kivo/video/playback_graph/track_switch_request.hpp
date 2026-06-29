#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

enum class TrackSwitchAtomicity {
    AtomicAllOrNothing,
    BestEffortPartialAllowed
};

struct AudioTrackSwitchRequest {
    std::uint64_t track_id{0};
};

struct VideoTrackSwitchRequest {
    std::uint64_t track_id{0};
};

struct AvTrackSetSwitchRequest {
    std::uint64_t audio_track_id{0};
    std::uint64_t video_track_id{0};
    TrackSwitchAtomicity atomicity{TrackSwitchAtomicity::AtomicAllOrNothing};
};

}  // namespace kivo::video::playback_graph
