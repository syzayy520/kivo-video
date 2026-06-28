// audio_output_format_decision.hpp — P6A §19: format negotiation result
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/output/audio_output_mode.hpp"
namespace kivo::video::audio_plane::output {
struct AudioOutputFormatDecision {
    ContractMetadata metadata;
    AudioOutputMode mode{AudioOutputMode::Unknown};
    int32_t sample_rate_hz{0};
    int32_t channel_count{0};
    int32_t bit_depth{0};
    int32_t valid_bits{0};
    int32_t channel_layout{0};
    bool bit_perfect_eligible{false};
    bool resampler_required{false};
    uint64_t evidence_id{0};
};
}  // namespace kivo::video::audio_plane::output
