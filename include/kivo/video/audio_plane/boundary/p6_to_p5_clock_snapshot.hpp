// p6_to_p5_clock_snapshot.hpp — P6A boundary: P6→P5 clock snapshot (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P6ToP5ClockSnapshot
// Snapshot of the P6 audio media plane clock state pushed to the P5 native
// decode layer. device_consumed_media_time_ms is signed to allow pre-roll
// deltas. clock_candidate is an enum-as-int placeholder for the active clock
// source selection.
struct P6ToP5ClockSnapshot {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t engine_epoch{0};
    uint64_t device_generation{0};
    int64_t device_consumed_media_time_ms{0};
    int64_t clock_error_bound_ms{0};
    int32_t clock_candidate{0};

    bool operator==(const P6ToP5ClockSnapshot&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary
