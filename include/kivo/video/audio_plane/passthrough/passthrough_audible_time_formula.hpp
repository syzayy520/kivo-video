// passthrough_audible_time_formula.hpp — P6A passthrough: PassthroughAudibleTimeFormula (§8.8)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
struct PassthroughAudibleTimeFormula {
    ContractMetadata metadata;
    int64_t media_time_of_last_consumed_burst{0};
    int64_t consumed_fraction_of_current_unit{0};
    int64_t external_decoder_latency_ms{0};
    int64_t user_sync_offset_ms{0};
    int64_t estimated_audible_now_media_time{0};
    int32_t confidence{0};  // 0=High, 1=Medium, 2=Low, 3=Invalid
    bool operator==(const PassthroughAudibleTimeFormula&) const = default;
};
}  // namespace kivo::video::audio_plane::passthrough
