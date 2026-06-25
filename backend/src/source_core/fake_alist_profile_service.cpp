#include "kivo/cinema_engine/source_core/fake_alist_profile_service.hpp"

namespace kivo::cinema_engine {

AListServerProfile FakeAListProfileService::build_standard_profile() {
    AListServerProfile p;
    p.metadata.schema_version = "1.1";
    p.profile_id = "alist-standard-001";
    p.server_version = "alist/3.0";
    p.site_name = "AList Drive";
    p.enable_sign = false;
    p.enable_web_dav = true;
    p.enable_sso = false;
    p.auth_method = "token";
    p.max_file_size = 10737418240;  // 10 GB
    p.total_storage = 1099511627776;  // 1 TB
    return p;
}

AListServerProfile FakeAListProfileService::build_webdav_enabled_profile() {
    AListServerProfile p;
    p.metadata.schema_version = "1.1";
    p.profile_id = "alist-webdav-001";
    p.server_version = "alist/3.1";
    p.site_name = "AList WebDAV";
    p.enable_sign = true;
    p.enable_web_dav = true;
    p.enable_sso = true;
    p.auth_method = "sso";
    p.max_file_size = 5368709120;  // 5 GB
    p.total_storage = 549755813888;  // 512 GB
    return p;
}

AListServerProfile FakeAListProfileService::build_restricted_profile() {
    AListServerProfile p;
    p.metadata.schema_version = "1.1";
    p.profile_id = "alist-restricted-001";
    p.server_version = "alist/2.9";
    p.site_name = "Restricted AList";
    p.enable_sign = false;
    p.enable_web_dav = false;
    p.enable_sso = false;
    p.auth_method = "password";
    p.max_file_size = 1073741824;  // 1 GB
    p.total_storage = 10737418240;  // 10 GB
    return p;
}

AListApiBehavior FakeAListProfileService::build_full_api_behavior() {
    AListApiBehavior b;
    b.metadata.schema_version = "1.1";
    b.behavior_id = "alist-api-full-001";
    b.supports_list = true;
    b.supports_get = true;
    b.supports_search = true;
    b.supports_rename = true;
    b.supports_move = true;
    b.supports_copy = true;
    b.supports_delete = true;
    b.supports_mkdir = true;
    b.supports_link_generation = true;
    return b;
}

AListApiBehavior FakeAListProfileService::build_readonly_api_behavior() {
    AListApiBehavior b;
    b.metadata.schema_version = "1.1";
    b.behavior_id = "alist-api-readonly-001";
    b.supports_list = true;
    b.supports_get = true;
    b.supports_search = true;
    b.supports_rename = false;
    b.supports_move = false;
    b.supports_copy = false;
    b.supports_delete = false;
    b.supports_mkdir = false;
    b.supports_link_generation = true;
    return b;
}

AListApiBehavior FakeAListProfileService::build_minimal_api_behavior() {
    AListApiBehavior b;
    b.metadata.schema_version = "1.1";
    b.behavior_id = "alist-api-minimal-001";
    b.supports_list = true;
    b.supports_get = true;
    b.supports_search = false;
    b.supports_rename = false;
    b.supports_move = false;
    b.supports_copy = false;
    b.supports_delete = false;
    b.supports_mkdir = false;
    b.supports_link_generation = false;
    return b;
}

AListServerFailure FakeAListProfileService::build_auth_expired_failure() {
    AListServerFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = AListFailureKind::AuthExpired;
    f.failure_reason = "token expired";
    f.retry_after_ms = 0;
    f.is_recoverable = true;
    return f;
}

AListServerFailure FakeAListProfileService::build_rate_limited_failure() {
    AListServerFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = AListFailureKind::RateLimited;
    f.failure_reason = "rate limit exceeded, too many requests";
    f.retry_after_ms = 60000;  // 1 minute
    f.is_recoverable = true;
    return f;
}

AListServerFailure FakeAListProfileService::build_backend_unavailable_failure() {
    AListServerFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = AListFailureKind::BackendUnavailable;
    f.failure_reason = "storage backend unreachable";
    f.retry_after_ms = 300000;  // 5 minutes
    f.is_recoverable = true;
    return f;
}

AListServerFailure FakeAListProfileService::build_permission_denied_failure() {
    AListServerFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = AListFailureKind::PermissionDenied;
    f.failure_reason = "insufficient permissions for this operation";
    f.retry_after_ms = 0;
    f.is_recoverable = false;
    return f;
}

AListServerFailure FakeAListProfileService::build_not_found_failure() {
    AListServerFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = AListFailureKind::NotFound;
    f.failure_reason = "resource not found";
    f.retry_after_ms = 0;
    f.is_recoverable = false;
    return f;
}

}  // namespace kivo::cinema_engine
