#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Inspector runtime contract describing what data sources the
// playback inspector reads and what capabilities it exposes.
struct InspectorRuntimeContract {
    ContractMetadata metadata;
    std::string inspector_id;
    bool reads_decision_ledger{true};
    bool reads_telemetry_timeline{true};
    bool reads_capability_join{true};
    bool supports_redaction{true};
};

}  // namespace kivo::cinema_engine
