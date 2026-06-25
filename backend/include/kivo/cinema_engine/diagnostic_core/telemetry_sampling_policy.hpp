#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Policy governing telemetry sampling rates and hot-path restrictions.
struct TelemetrySamplingPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    int64_t max_sample_rate_hz{100};
    bool allow_hot_path{false};
    int64_t ring_buffer_capacity{10000};
};

}  // namespace kivo::cinema_engine
