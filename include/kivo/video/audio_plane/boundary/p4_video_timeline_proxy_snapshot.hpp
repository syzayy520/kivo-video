// p4_video_timeline_proxy_snapshot.hpp — P6A boundary: P4 video timeline proxy snapshot (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P4VideoTimelineProxySnapshot
// Proxy timeline snapshot representing the video plane's media time as seen by
// the P4 orchestration layer. Used by the audio media plane (P6) for A/V sync
// reasoning without a direct dependency on the video plane. When
// derived_from_p5_evidence is true, the snapshot was reconstructed from P5
// decode evidence rather than a live video clock.
struct P4VideoTimelineProxySnapshot {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t engine_epoch{0};
    int64_t video_media_time_ms{0};
    int64_t error_bound_ms{0};
    bool derived_from_p5_evidence{false};

    bool operator==(const P4VideoTimelineProxySnapshot&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary
