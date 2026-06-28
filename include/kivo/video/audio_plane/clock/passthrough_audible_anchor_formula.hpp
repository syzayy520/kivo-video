// passthrough_audible_anchor_formula.hpp — P6A clock: PassthroughAudibleAnchorFormula
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct PassthroughAudibleAnchorFormula {
    ContractMetadata metadata;
    int64_t media_time_of_last_consumed_burst_ms{0};
    int64_t consumed_fraction_of_current_unit_ms{0};
    int64_t external_decoder_latency_ms{0};
    int64_t user_sync_offset_ms{0};
    int64_t estimate_ms{0};
    bool operator==(const PassthroughAudibleAnchorFormula&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
