#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Result of a remote direct play gate check.
struct RemoteDirectPlayGateResult {
    ContractMetadata metadata;
    std::string request_id;
    bool gate_passed{false};
    bool transcode_requires_approval{false};
    bool credentials_redacted{false};
    bool redirect_leak_detected{false};
    bool cache_binding_valid{false};
    std::string failure_reason;
};

}  // namespace kivo::cinema_engine
