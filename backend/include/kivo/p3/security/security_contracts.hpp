#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::p3::security {

/// NetworkPolicy SSRF: enforce scheme/host/IP policies.
/// No default private IP access. No auth forwarded cross-origin by default.
struct NetworkPolicy
{
    std::string schema_version{"p3.security.network_policy.v1"};
    std::string allowed_schemes{"https"};
    std::string blocked_schemes{"file"};
    bool block_private_ips{true};
    bool block_loopback{true};
    bool block_link_local{true};
    bool block_multicast{true};
    int32_t max_redirects{5};
    int32_t max_cname_depth{5};
    int32_t max_dns_attempts{3};
    bool cross_origin_redirect_allowed{false};
    bool final_connected_ip_validation{true};
    std::string tls_policy{};
    std::string proxy_policy{};
};

/// CredentialRefreshSingleFlight: one refresh per credential scope.
struct CredentialSingleFlight
{
    std::string schema_version{"p3.security.credential_single_flight.v1"};
    std::string credential_scope{};
    bool refresh_in_progress{false};
    int32_t waiter_count{0};
    bool all_waiters_cancelled{false};

    bool can_join() const noexcept { return refresh_in_progress && !all_waiters_cancelled; }
};

/// SignedUrlHandle: url_material_ref for real I/O, redacted_url for evidence.
struct SignedUrlHandle
{
    std::string schema_version{"p3.security.signed_url.v1"};
    std::string url_material_ref{};
    std::string redacted_url{};
    int64_t expiry_ns{0};
    std::string refresh_ownership{};

    bool is_expired(int64_t now_ns) const noexcept { return now_ns >= expiry_ns; }
    bool has_material() const noexcept { return !url_material_ref.empty(); }
};

/// SecureMemory state: make credential memory handling auditable.
enum class SecureMemoryState
{
    zeroized_after_use,
    best_effort_with_reason,
    unsupported_blocking
};

struct SecureMemoryPolicy
{
    std::string schema_version{"p3.security.secure_memory.v1"};
    SecureMemoryState token_state{SecureMemoryState::zeroized_after_use};
    SecureMemoryState signed_url_state{SecureMemoryState::zeroized_after_use};
    SecureMemoryState password_state{SecureMemoryState::zeroized_after_use};
    std::string best_effort_reason{};
    bool is_audited{false};

    bool is_valid_for_supported_path() const noexcept
    {
        return token_state != SecureMemoryState::unsupported_blocking;
    }
};

/// RedactionPolicy: ensure sensitive data is sanitized everywhere.
struct RedactionPolicy
{
    std::string schema_version{"p3.security.redaction.v1"};
    bool redact_url_query{true};
    bool redact_auth_header{true};
    bool redact_cookie{true};
    bool redact_tokens{true};
    bool redact_signed_url{true};
    bool redact_password{true};
    bool redact_username{true};
    bool redact_private_host{true};
    bool redact_user_path{true};
    bool redact_provider_account{true};
    bool redact_session_ids{true};
    bool redact_media_title_hints{true};
};

} // namespace kivo::p3::security
