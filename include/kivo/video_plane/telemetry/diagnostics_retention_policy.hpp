// diagnostics_retention_policy.hpp — GATE-028: P5A diagnostics retention policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::telemetry {

struct DiagnosticsRetentionPolicy {
    uint64_t max_retention_days{30};        // max days to retain diagnostics
    uint64_t max_storage_bytes{0};          // max storage for diagnostics
    bool auto_delete_on_expiry{true};       // auto-delete expired diagnostics
    std::string deletion_method;            // how diagnostics are deleted
    bool operator==(DiagnosticsRetentionPolicy const&) const = default;
};

}  // namespace kivo::video_plane::telemetry
