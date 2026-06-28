// pcm_tail_time_formula.hpp — P6A clock: PcmTailTimeFormula
// endpoint_tail = consumed + padding
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct PcmTailTimeFormula {
    ContractMetadata metadata;
    int64_t device_consumed_media_time_ms{0};
    int64_t device_padding_duration_ms{0};
    int64_t endpoint_tail_ms{0};
    bool operator==(const PcmTailTimeFormula&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
