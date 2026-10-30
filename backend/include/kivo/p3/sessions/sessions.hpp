#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include "kivo/p3/source_model/source_kinds.hpp"

namespace kivo::p3::sessions {

/// SourceSessionPlan defines source session plan inputs and policies.
/// No raw URL in evidence. No tell() API. content_encoding_policy mandatory.
/// schema_version: p3.session.source_plan.v1
struct SourceSessionPlan
{
    std::string schema_version{"p3.session.source_plan.v1"};
    std::string plan_id{};
    std::string identity_ref{};
    std::string auth_ref{};
    std::string url_material_ref{};
    std::string redacted_url{};
    std::string required_headers_ref{};
    std::string redacted_headers_ref{};
    kivo::p3::source_model::SourceTransportKind transport_kind{};
    std::string range_policy{};
    std::string seek_policy{};
    std::string content_encoding_policy{};
    std::string connection_policy{};
    std::string preconnect_policy{};
    std::string proxy_policy{};
    std::string tls_policy{};
    std::string expiry_policy{};
    std::string refresh_policy{};
    std::string cancellation_policy{};
    std::string resource_policy{};
    std::string reader_policy{};

    bool is_valid() const noexcept { return !plan_id.empty() && !content_encoding_policy.empty(); }
};

/// SourceSession lifecycle: single-reader default, read/seek not concurrent.
/// close/abort concurrent requests but idempotent. read returns start/end offset.
struct SourceSession
{
    std::string schema_version{"p3.session.source.v1"};
    std::string session_id{};
    std::string plan_ref{};
    bool is_open{false};
    int64_t current_offset{0};
    int64_t content_length{0};
    bool supports_independent_reader{false};

    bool can_read() const noexcept { return is_open; }
    bool can_seek() const noexcept { return is_open; }
};

/// IndependentRangeReader: independent reader without shared mutable seek position.
struct IndependentRangeReader
{
    std::string schema_version{"p3.session.independent_reader.v1"};
    std::string reader_id{};
    std::string session_ref{};
    bool is_active{false};
    bool has_strong_session_ref{false};
    bool lease_managed{false};

    bool is_valid() const noexcept { return !reader_id.empty() && is_active; }
};

/// ManifestSessionPlan: HLS/DASH manifest plan fields.
/// No segment queue. No ABR execution. Refresh ownership mandatory.
struct ManifestSessionPlan
{
    std::string schema_version{"p3.session.manifest_plan.v1"};
    std::string plan_id{};
    kivo::p3::source_model::MediaDeliveryKind delivery_kind{};
    std::string fetch_policy{};
    std::string refresh_policy{};
    std::string staleness_policy{};
    std::string snapshot_policy{};
    std::string segment_identity_policy{};
    std::string content_encoding_policy{};
    std::string redirect_policy{};
    std::string refresh_ownership{};
    std::string cancellation_policy{};

    bool is_valid() const noexcept { return !plan_id.empty() && !refresh_ownership.empty(); }
};

/// ManifestSession snapshot supersede policy.
struct ManifestSnapshotSupersede
{
    std::string schema_version{"p3.session.snapshot_supersede.v1"};
    int32_t snapshot_version{0};
    std::string supersede_policy{"keep_current_until_p4_ack"};
    bool old_snapshot_destroyed{false};

    bool has_supersede_policy() const noexcept { return !supersede_policy.empty(); }
};

/// ManifestSessionLease ownership: refresh responsibility values.
enum class ManifestRefreshOwnership
{
    p3_refreshes_until_transfer,
    p4_refreshes_after_transfer,
    shared_refresh_via_runtime,
    reopen_by_plan_only
};

/// PlaylistSessionPlan: M3U, M3U8 simple, PLS, unknown.
struct PlaylistSessionPlan
{
    std::string schema_version{"p3.session.playlist_plan.v1"};
    std::string plan_id{};
    std::string playlist_kind{};
    std::string auth_ref{};
    std::string entry_resolution_policy{};
    int32_t max_recursion_depth{0};
    int32_t max_entries{0};
    bool allow_nested_playlist{false};
    std::string mixed_scheme_policy{};
    std::string cancellation_policy{};

    bool is_valid() const noexcept { return !plan_id.empty(); }
};

/// SessionErrorObserver: propagate errors across P3/P4 boundaries safely.
struct SessionErrorObserver
{
    std::string schema_version{"p3.session.error_observer.v1"};
    std::string observer_id{};
    std::string weak_token{};
    bool auto_unregister{false};
    bool is_registered{false};

    bool is_valid() const noexcept { return !observer_id.empty() && is_registered; }
};

/// IndependentAfterTransfer: responsibility after P4 takes lease.
struct IndependentAfterTransfer
{
    std::string schema_version{"p3.session.independent_after_transfer.v1"};
    bool p4_owns_close{false};
    bool p4_owns_abort{false};
    std::string error_routing_policy{};

    bool is_valid() const noexcept { return p4_owns_close || p4_owns_abort; }
};

} // namespace kivo::p3::sessions
