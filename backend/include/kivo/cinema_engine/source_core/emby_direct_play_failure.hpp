#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Failure kinds for Emby direct play operations.
enum class EmbyDirectPlayFailureKind {
    ItemNotFound,
    IncompatibleFormat,
    ServerError,
};

// Failure result from a direct play operation.
struct EmbyDirectPlayFailure {
    ContractMetadata metadata;
    EmbyDirectPlayFailureKind failure_kind;
    std::string failure_reason;
    int64_t retry_after_ms{0};
    bool is_recoverable{false};
};

}  // namespace kivo::cinema_engine
