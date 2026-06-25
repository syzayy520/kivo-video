#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Failure classification for source adapter operations.
struct SourceAdapterFailure {
    ContractMetadata metadata;
    std::string adapter_id;
    std::string failure_kind;   // "connection_lost", "auth_failed", "timeout", "range_not_satisfiable", "redirect_loop", "tls_error", "not_found", "permission_denied"
    std::string failure_reason;
    bool is_recoverable{false};
    int retry_after_ms{0};
};

}  // namespace kivo::cinema_engine
