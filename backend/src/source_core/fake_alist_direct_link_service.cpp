#include "kivo/cinema_engine/source_core/fake_alist_direct_link_service.hpp"

namespace kivo::cinema_engine {

// --- Request builders ---

AListDirectLinkRequest FakeAListDirectLinkService::build_simple_request() {
    AListDirectLinkRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dl-req-001";
    r.file_path = "/movies/test.mp4";
    r.auth_token = "alist-token-abc123";
    r.expires_in_seconds = 0;  // server default
    return r;
}

AListDirectLinkRequest FakeAListDirectLinkService::build_custom_ttl_request() {
    AListDirectLinkRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dl-req-002";
    r.file_path = "/movies/long-movie.mkv";
    r.auth_token = "alist-token-def456";
    r.expires_in_seconds = 3600;  // 1 hour
    return r;
}

// --- Response builders ---

AListDirectLinkResponse FakeAListDirectLinkService::build_successful_link_response() {
    AListDirectLinkResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dl-req-001";
    r.direct_link_url = "https://alist.example.com/d/test.mp4?token=xyz789";
    r.ttl_seconds = 7200;  // 2 hours (server default)
    r.content_type = "video/mp4";
    r.content_length = 1073741824;  // 1 GB
    return r;
}

AListDirectLinkResponse FakeAListDirectLinkService::build_short_ttl_response() {
    AListDirectLinkResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dl-req-003";
    r.direct_link_url = "https://alist.example.com/d/short.mp4?token=short123";
    r.ttl_seconds = 300;  // 5 minutes
    r.content_type = "video/mp4";
    r.content_length = 524288000;  // 500 MB
    return r;
}

AListDirectLinkResponse FakeAListDirectLinkService::build_no_ttl_response() {
    AListDirectLinkResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dl-req-004";
    r.direct_link_url = "https://alist.example.com/d/nottl.mp4?token=nottl000";
    r.ttl_seconds = 0;  // no TTL specified
    r.content_type = "video/mp4";
    r.content_length = 268435456;  // 256 MB
    return r;
}

// --- Identity builders ---

AListDirectLinkIdentity FakeAListDirectLinkService::build_initial_identity() {
    AListDirectLinkIdentity id;
    id.metadata.schema_version = "1.1";
    id.identity_id = "alist-identity-001";
    id.file_path = "/movies/test.mp4";
    id.content_hash = "sha256-abc123def456";
    id.first_link_generated_at = 1719200000;
    id.link_refresh_count = 0;
    id.last_refresh_at = 0;
    return id;
}

AListDirectLinkIdentity FakeAListDirectLinkService::build_refreshed_identity() {
    AListDirectLinkIdentity id;
    id.metadata.schema_version = "1.1";
    id.identity_id = "alist-identity-001";  // stable across refreshes
    id.file_path = "/movies/test.mp4";
    id.content_hash = "sha256-abc123def456";  // same content
    id.first_link_generated_at = 1719200000;
    id.link_refresh_count = 1;
    id.last_refresh_at = 1719207200;  // 2 hours later
    return id;
}

AListDirectLinkIdentity FakeAListDirectLinkService::build_multi_refresh_identity() {
    AListDirectLinkIdentity id;
    id.metadata.schema_version = "1.1";
    id.identity_id = "alist-identity-001";  // stable across refreshes
    id.file_path = "/movies/test.mp4";
    id.content_hash = "sha256-abc123def456";  // same content
    id.first_link_generated_at = 1719200000;
    id.link_refresh_count = 5;
    id.last_refresh_at = 1719214400;  // many refreshes later
    return id;
}

// --- Failure builders ---

AListDirectLinkFailure FakeAListDirectLinkService::build_link_expired_failure() {
    AListDirectLinkFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = AListDirectLinkFailureKind::LinkExpired;
    f.failure_reason = "direct link has expired, request a new one";
    f.retry_after_ms = 0;
    f.is_recoverable = true;
    return f;
}

AListDirectLinkFailure FakeAListDirectLinkService::build_server_unreachable_failure() {
    AListDirectLinkFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = AListDirectLinkFailureKind::ServerUnreachable;
    f.failure_reason = "AList server is unreachable";
    f.retry_after_ms = 30000;  // 30 seconds
    f.is_recoverable = true;
    return f;
}

AListDirectLinkFailure FakeAListDirectLinkService::build_auth_denied_failure() {
    AListDirectLinkFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = AListDirectLinkFailureKind::AuthDenied;
    f.failure_reason = "authentication token is invalid or expired";
    f.retry_after_ms = 0;
    f.is_recoverable = false;
    return f;
}

}  // namespace kivo::cinema_engine
