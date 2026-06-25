#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Failure kinds for AList direct link operations.
enum class AListDirectLinkFailureKind {
    LinkExpired,
    ServerUnreachable,
    AuthDenied,
    RateLimited,
    FilePathInvalid,
};

// Failure result from a direct link operation.
struct AListDirectLinkFailure {
    ContractMetadata metadata;
    AListDirectLinkFailureKind failure_kind;
    std::string failure_reason;
    int64_t retry_after_ms{0};
    bool is_recoverable{false};
};

}  // namespace kivo::cinema_engine
