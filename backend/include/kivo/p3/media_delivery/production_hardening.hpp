#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::media_delivery {

/// WebDavProductionHardening defines WebDAV hardening contracts.
/// P2 not_verified items must be upgraded or honestly deferred here.
/// schema_version: p3.webdav.hardening.v1
struct WebDavProductionHardening
{
    std::string schema_version{"p3.webdav.hardening.v1"};
    bool href_exact_match_verified{false};      // P2: not_verified
    bool collection_rejection_verified{false};   // P2: not_verified
    bool malformed_xml_fail_closed_verified{false}; // P2: not_verified
    bool large_xml_body_cap_verified{false};     // P2: not_verified
    std::string propfind_depth_policy{"depth_1_default"};
    std::string status_taxonomy{};
    std::string redirect_policy{};
    std::string auth_redaction_policy{};
    std::string range_policy{};
    std::string validator_policy{};
    std::string path_encoding_policy{};

    bool all_p2_not_verified_resolved() const noexcept
    {
        return href_exact_match_verified && collection_rejection_verified
            && malformed_xml_fail_closed_verified && large_xml_body_cap_verified;
    }
};

/// HttpProductionTransport defines HTTP transport boundary hardening.
/// schema_version: p3.http.transport.v1
struct HttpProductionTransport
{
    std::string schema_version{"p3.http.transport.v1"};
    bool head_supported{true};
    bool get_range_supported{true};
    bool response_206_supported{true};
    bool response_200_no_range_not_seekable{true};
    bool response_416_semantics_supported{true};
    bool redirect_revalidation{true};
    bool auth_cookie_redaction{true};
    bool content_length_evidence{true};
    bool content_type_evidence{true};
    bool etag_evidence{true};
    bool last_modified_evidence{true};
    bool short_read_source_changed{true};
    bool timeout_cancel_taxonomy{true};
    bool retry_after_handling{true};
    bool dns_ssrf_tls_proxy{true};
    bool connection_pool{true};
    bool preconnect{true};
    bool identity_range_encoding{true};
    bool final_connected_ip_validation{true};

    bool is_identity_encoding_only() const noexcept { return identity_range_encoding; }
};

} // namespace kivo::p3::media_delivery
