#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::p3::evidence_contracts {

/// FastStartEvidence: fast start evidence with confidence.
struct FastStartEvidence
{
    std::string schema_version{"p3.evidence.fast_start.v1"};
    std::string evidence_id{};
    bool fast_start_possible{false};
    double confidence{0.0};
    std::string constraints_ref{};
    int64_t estimated_start_latency_ns{0};
};

/// ManifestEvidence: manifest evidence.
struct ManifestEvidence
{
    std::string schema_version{"p3.evidence.manifest.v1"};
    std::string evidence_id{};
    int32_t snapshot_version{0};
    bool is_stale{false};
};

/// PlaylistEvidence: playlist evidence.
struct PlaylistEvidence
{
    std::string schema_version{"p3.evidence.playlist.v1"};
    std::string evidence_id{};
    int32_t entry_count{0};
    std::string recursion_policy_ref{};
};

/// EvidencePerformancePolicy: keep evidence from blocking success path.
struct EvidencePerformancePolicy
{
    std::string schema_version{"p3.evidence.performance.v1"};
    int64_t success_path_sync_max_ns{5'000'000};
    int64_t success_path_disk_max_ns{0};
    bool unbounded_growth_blocked{true};
};

/// GoldenDiffPolicy: make golden evidence reproducible.
struct GoldenDiffPolicy
{
    std::string schema_version{"p3.evidence.golden_diff.v1"};
    bool ignore_field_order{true};
    bool mask_timestamps{true};
    bool mask_ports{true};
    bool mask_paths{true};
    bool mask_operation_ids{true};
    double numeric_tolerance{0.001};
    bool no_current_commit_hash_in_evidence{true};
};

} // namespace kivo::p3::evidence_contracts
