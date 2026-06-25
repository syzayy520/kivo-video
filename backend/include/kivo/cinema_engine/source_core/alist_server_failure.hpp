#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// AList failure types.
enum class AListFailureKind {
    AuthExpired,
    RateLimited,
    BackendUnavailable,
    PermissionDenied,
    NotFound
};

// AList server failure information.
struct AListServerFailure {
    ContractMetadata metadata;
    AListFailureKind failure_kind{AListFailureKind::AuthExpired};
    std::string failure_reason;
    int64_t retry_after_ms{0};
    bool is_recoverable{false};
};

}  // namespace kivo::cinema_engine
