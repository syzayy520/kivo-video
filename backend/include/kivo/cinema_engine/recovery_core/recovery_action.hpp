#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Type of recovery action taken when a failure occurs.
enum class RecoveryActionKind {
    Retry,
    Fallback,
    Abort,
    RestartComponent,
    SkipTrack
};

// Severity of the error that triggered recovery.
enum class RecoveryTriggerSeverity {
    Warning,
    Error,
    Fatal
};

// Describes a single recovery action taken when a failure occurs.
struct RecoveryAction {
    ContractMetadata metadata;
    std::string action_id;
    RecoveryActionKind action_kind{RecoveryActionKind::Retry};
    std::string trigger_error_domain;
    RecoveryTriggerSeverity trigger_error_severity{RecoveryTriggerSeverity::Error};
    int max_retries{3};
    int64_t retry_delay_ms{1000};
    std::string user_visible_hint;
};

}  // namespace kivo::cinema_engine
