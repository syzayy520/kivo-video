// p6_to_p5_sync_indicator.hpp — P6A boundary: P6→P5 sync indicator (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P6ToP5SyncIndicator
// Audio/video synchronization indicator reported from the P6 audio media plane
// to the P5 native decode layer. drift_ms is signed (audio ahead = positive).
// confidence is an enum-as-int placeholder for sync-confidence grading.
struct P6ToP5SyncIndicator {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int64_t audio_audible_now_ms{0};
    int64_t video_audible_now_ms{0};
    int64_t drift_ms{0};
    int32_t confidence{0};
    bool in_sync{false};

    bool operator==(const P6ToP5SyncIndicator&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary
