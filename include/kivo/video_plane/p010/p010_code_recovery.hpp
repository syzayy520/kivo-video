// p010_code_recovery.hpp — GATE-015: P5A P010 code recovery
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::p010 {

struct P010CodeRecovery {
    bool recovery_attempted{false};         // recovery was attempted
    bool recovery_succeeded{false};         // recovery succeeded
    std::string recovery_strategy;          // strategy used for recovery
    std::string failure_reason;             // reason if recovery failed
    uint32_t retry_count{0};                // number of retries
    bool operator==(P010CodeRecovery const&) const = default;
};

}  // namespace kivo::video_plane::p010
