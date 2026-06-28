// p4_synthetic_timeline_snapshot.hpp — P6A boundary: P4 synthetic timeline snapshot (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P4SyntheticTimelineSnapshot
// Synthetic timeline authority snapshot produced by the P4 orchestration layer
// when no real device clock is available (e.g., during pre-roll or recovery).
// is_authority indicates whether downstream consumers must treat this snapshot
// as the authoritative timeline source.
struct P4SyntheticTimelineSnapshot {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t engine_epoch{0};
    int64_t synthetic_media_time_ms{0};
    int64_t error_bound_ms{0};
    bool is_authority{false};

    bool operator==(const P4SyntheticTimelineSnapshot&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary
