// pcm_current_audible_time_formula.hpp — P6A clock: PcmCurrentAudibleTimeFormula
// estimate = consumed - endpoint_lat - hw_transport + codec_trim + user_offset
// valid interval [estimate-bound, estimate+bound]
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct PcmCurrentAudibleTimeFormula {
    ContractMetadata metadata;
    int64_t device_consumed_media_time_ms{0};
    int64_t endpoint_latency_ms{0};
    int64_t hardware_transport_latency_ms{0};
    int64_t codec_delay_trimmed_offset_ms{0};
    int64_t user_sync_offset_ms{0};
    int64_t estimate_ms{0};
    int64_t error_bound_ms{0};
    bool operator==(const PcmCurrentAudibleTimeFormula&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
