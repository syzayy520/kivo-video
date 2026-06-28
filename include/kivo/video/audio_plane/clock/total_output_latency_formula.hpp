// total_output_latency_formula.hpp — P6A clock: TotalOutputLatencyFormula
// total = render + device + endpoint + hw + processing +/- error_bound
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct TotalOutputLatencyFormula {
    ContractMetadata metadata;
    int64_t render_queue_latency_ms{0};
    int64_t device_buffer_latency_ms{0};
    int64_t endpoint_latency_ms{0};
    int64_t hardware_transport_latency_ms{0};
    int64_t processing_backend_latency_ms{0};
    int64_t clock_error_bound_ms{0};
    int64_t total_ms{0};
    bool operator==(const TotalOutputLatencyFormula&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
