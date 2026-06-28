// audio_output_open_request.hpp — P6A §5/§19: open request
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/output/audio_output_mode.hpp"
#include "kivo/video/audio_plane/output/audio_output_device_id.hpp"
namespace kivo::video::audio_plane::output {
struct AudioOutputOpenRequest {
    ContractMetadata metadata;
    AudioOutputDeviceId device_id;
    AudioOutputMode requested_mode{AudioOutputMode::Unknown};
    int32_t sample_rate_hz{0};
    int32_t channel_count{0};
    int32_t bit_depth{0};
    int32_t buffer_period_ms{0};
    bool event_driven{true};
};
}  // namespace kivo::video::audio_plane::output
