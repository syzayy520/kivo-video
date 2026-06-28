// p4_video_timeline_proxy_clock.hpp — P6A clock: P4VideoTimelineProxyClock
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct P4VideoTimelineProxyClock {
    ContractMetadata metadata;
    int64_t video_media_time_ms{0};
    int64_t error_bound_ms{0};
    bool derived_from_p5_evidence{false};
    bool is_authority{false};
    bool operator==(const P4VideoTimelineProxyClock&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
