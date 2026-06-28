// external_live_clock_source.hpp — P6A clock: ExternalLiveClockSource
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct ExternalLiveClockSource {
    ContractMetadata metadata;
    int64_t estimate_ms{0};
    int64_t error_bound_ms{0};
    bool is_available{false};
    bool operator==(const ExternalLiveClockSource&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
