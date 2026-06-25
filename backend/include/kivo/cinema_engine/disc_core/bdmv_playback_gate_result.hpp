#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct BdmvPlaybackGateResult {
    ContractMetadata metadata;
    std::string request_id;
    bool success{false};
    bool playback_allowed{false};
    std::string gate_reason;
    std::string failure_reason;
};

} // namespace kivo::cinema_engine
