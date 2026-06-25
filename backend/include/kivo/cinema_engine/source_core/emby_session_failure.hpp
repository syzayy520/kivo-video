#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Failure kinds for Emby session operations.
enum class EmbySessionFailureKind {
    SessionExpired,
    ServerUnreachable,
    ItemNotFound,
    AccessDenied,
};

// Failure result from a session operation.
struct EmbySessionFailure {
    ContractMetadata metadata;
    EmbySessionFailureKind failure_kind;
    std::string failure_reason;
    int64_t retry_after_ms{0};
    bool is_recoverable{false};
};

}  // namespace kivo::cinema_engine
