// audio_output_open_result.hpp — P6A §5: open result
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/output/audio_output_mode.hpp"
namespace kivo::video::audio_plane::output {
enum class AudioOutputOpenStatus : int32_t { Ok = 0, Failed = 1, FormatNotSupported = 2, DeviceBusy = 3, InvalidRequest = 4 };
struct AudioOutputOpenResult {
    ContractMetadata metadata;
    AudioOutputOpenStatus status{AudioOutputOpenStatus::Failed};
    AudioOutputMode actual_mode{AudioOutputMode::Unknown};
    int32_t actual_sample_rate_hz{0};
    int32_t actual_channel_count{0};
    int32_t actual_bit_depth{0};
    int32_t actual_buffer_period_ms{0};
    uint64_t evidence_id{0};
};
}  // namespace kivo::video::audio_plane::output
