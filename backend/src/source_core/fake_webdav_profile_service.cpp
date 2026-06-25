#include "kivo/cinema_engine/source_core/fake_webdav_profile_service.hpp"

namespace kivo::cinema_engine {

WebDavServerProfile FakeWebDavProfileService::build_nginx_profile() {
    WebDavServerProfile p;
    p.metadata.schema_version = "1.1";
    p.profile_id = "webdav-nginx-001";
    p.server_version = "nginx/1.24.0";
    p.dav_compliance = "1";
    p.supports_range_read = true;
    p.supports_head_probe = true;
    p.supports_propfind = true;
    p.supports_lock_unlock = false;
    p.auth_method = "basic";
    p.max_content_length = 5368709120;  // 5 GB
    return p;
}

WebDavServerProfile FakeWebDavProfileService::build_apache_profile() {
    WebDavServerProfile p;
    p.metadata.schema_version = "1.1";
    p.profile_id = "webdav-apache-001";
    p.server_version = "Apache/2.4.58";
    p.dav_compliance = "2";
    p.supports_range_read = true;
    p.supports_head_probe = true;
    p.supports_propfind = true;
    p.supports_lock_unlock = true;
    p.auth_method = "digest";
    p.max_content_length = 10737418240;  // 10 GB
    return p;
}

WebDavServerProfile FakeWebDavProfileService::build_rclone_profile() {
    WebDavServerProfile p;
    p.metadata.schema_version = "1.1";
    p.profile_id = "webdav-rclone-001";
    p.server_version = "rclone/v1.65.0";
    p.dav_compliance = "1";
    p.supports_range_read = true;
    p.supports_head_probe = true;
    p.supports_propfind = true;
    p.supports_lock_unlock = false;
    p.auth_method = "basic";
    p.max_content_length = 0;  // unlimited
    return p;
}

WebDavServerProfile FakeWebDavProfileService::build_synology_profile() {
    WebDavServerProfile p;
    p.metadata.schema_version = "1.1";
    p.profile_id = "webdav-synology-001";
    p.server_version = "WebDAV/2.0";
    p.dav_compliance = "1";
    p.supports_range_read = true;
    p.supports_head_probe = false;
    p.supports_propfind = true;
    p.supports_lock_unlock = false;
    p.auth_method = "basic";
    p.max_content_length = 1073741824;  // 1 GB
    return p;
}

WebDavServerProfile FakeWebDavProfileService::build_no_range_profile() {
    WebDavServerProfile p;
    p.metadata.schema_version = "1.1";
    p.profile_id = "webdav-norange-001";
    p.server_version = "custom/1.0";
    p.dav_compliance = "1";
    p.supports_range_read = false;
    p.supports_head_probe = true;
    p.supports_propfind = false;
    p.supports_lock_unlock = false;
    p.auth_method = "bearer";
    p.max_content_length = 104857600;  // 100 MB
    return p;
}

WebDavRangeBehavior FakeWebDavProfileService::build_nginx_range_behavior() {
    WebDavRangeBehavior r;
    r.metadata.schema_version = "1.1";
    r.range_header_supported = true;
    r.content_range_present = true;
    r.single_byte_range_only = true;
    r.chunk_size_limit = 10485760;  // 10 MB
    r.status_on_full_content = 200;
    r.status_on_partial = 206;
    r.status_on_not_satisfiable = 416;
    return r;
}

WebDavRangeBehavior FakeWebDavProfileService::build_partial_range_behavior() {
    WebDavRangeBehavior r;
    r.metadata.schema_version = "1.1";
    r.range_header_supported = true;
    r.content_range_present = true;
    r.single_byte_range_only = false;
    r.chunk_size_limit = 1048576;  // 1 MB
    r.status_on_full_content = 200;
    r.status_on_partial = 206;
    r.status_on_not_satisfiable = 200;  // some servers return 200 instead of 416
    return r;
}

WebDavRangeBehavior FakeWebDavProfileService::build_no_range_behavior() {
    WebDavRangeBehavior r;
    r.metadata.schema_version = "1.1";
    r.range_header_supported = false;
    r.content_range_present = false;
    r.single_byte_range_only = false;
    r.chunk_size_limit = 0;
    r.status_on_full_content = 200;
    r.status_on_partial = 200;  // ignores range, returns full
    r.status_on_not_satisfiable = 200;
    return r;
}

WebDavServerFailure FakeWebDavProfileService::build_connection_failure() {
    WebDavServerFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = WebDavFailureKind::ConnectionRefused;
    f.failure_reason = "connection refused by remote server";
    f.is_recoverable = true;
    return f;
}

WebDavServerFailure FakeWebDavProfileService::build_auth_failure() {
    WebDavServerFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = WebDavFailureKind::AuthenticationRequired;
    f.failure_reason = "401 Unauthorized";
    f.is_recoverable = true;
    return f;
}

WebDavServerFailure FakeWebDavProfileService::build_range_not_supported_failure() {
    WebDavServerFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = WebDavFailureKind::RangeNotSupported;
    f.failure_reason = "server does not support Range header";
    f.is_recoverable = false;
    return f;
}

WebDavProbeResult FakeWebDavProfileService::build_successful_probe() {
    WebDavProbeResult r;
    r.metadata.schema_version = "1.1";
    r.profile_id = "probe-success-001";
    r.server_profile = build_nginx_profile();
    r.server_profile.profile_id = r.profile_id;
    r.range_behavior = build_nginx_range_behavior();
    r.probe_latency_ms = 120;
    r.probe_success = true;
    return r;
}

WebDavProbeResult FakeWebDavProfileService::build_failed_probe() {
    WebDavProbeResult r;
    r.metadata.schema_version = "1.1";
    r.profile_id = "probe-fail-001";
    r.server_profile = build_no_range_profile();
    r.server_profile.profile_id = r.profile_id;
    r.range_behavior = build_no_range_behavior();
    r.probe_latency_ms = 5000;
    r.probe_success = false;
    r.failure = build_connection_failure();
    return r;
}

}  // namespace kivo::cinema_engine
