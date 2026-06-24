#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// AudioEndpointRuntimeContract is the contract for platform-specific audio output.
// Actual platform types (audio session, buffer, endpoint) stay inside the adapter boundary.
struct AudioEndpointRuntimeContract {
    ContractMetadata metadata;
    std::string endpoint_id;
    std::string endpoint_name;
    std::string output_mode;  // "shared_pcm", "exclusive_pcm", "passthrough"
    int32_t sample_rate{0};
    int32_t channels{0};
    int32_t bit_depth{0};
    bool is_initialized{false};
    bool is_playing{false};
    int64_t samples_written{0};
    double buffer_duration_ms{0.0};
};

}  // namespace kivo::cinema_engine
