// audio_seek_evidence.hpp — P6A seek: audio seek evidence contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::seek {
struct AudioSeekEvidence {
    kivo::video::audio_plane::ContractMetadata metadata;
    uint64_t evidence_id{0};
    int64_t requested_time_ms{0};
    int64_t actual_decode_start_ms{0};
    int64_t actual_audible_start_ms{0};
    int32_t preroll_duration_ms{0};
    int32_t discarded_samples{0};
    int32_t codec_delay_recomputed{0};
    int64_t error_bound_ms{0};
};
}  // namespace kivo::video::audio_plane::seek
