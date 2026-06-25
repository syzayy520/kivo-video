#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Request to read inspection data from a playback session.
struct InspectorReadRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string inspector_id;
    std::string source_type;  // "snapshot", "ledger", "telemetry"
    std::string session_id;
    bool include_redacted{false};
};

}  // namespace kivo::cinema_engine
