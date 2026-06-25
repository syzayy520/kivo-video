#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Result of reading inspection data from a playback session.
struct InspectorReadResult {
    ContractMetadata metadata;
    std::string result_id;
    std::string request_id;
    std::string source_type;
    std::string data_payload;
    int64_t record_count{0};
    bool was_redacted{false};
};

}  // namespace kivo::cinema_engine
