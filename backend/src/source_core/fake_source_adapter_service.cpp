#include "kivo/cinema_engine/source_core/fake_source_adapter_service.hpp"

namespace kivo::cinema_engine {

// --- Local File Adapter ---

SourceAdapterIdentity FakeSourceAdapterService::build_local_file_identity() {
    SourceAdapterIdentity id;
    id.metadata.schema_version = "1.1";
    id.adapter_id = "local-file-001";
    id.adapter_kind = "local";
    id.protocol = "file";
    id.host = "";
    id.requires_auth = false;
    id.credential_scope = "";
    return id;
}

SourceAdapterCapability FakeSourceAdapterService::build_local_file_capability() {
    SourceAdapterCapability cap;
    cap.metadata.schema_version = "1.1";
    cap.adapter_id = "local-file-001";
    cap.supports_range_read = true;
    cap.supports_seek = true;
    cap.supports_list_directory = false;
    cap.supports_byte_range_cache = false;
    cap.supports_connection_reuse = false;
    cap.max_range_chunk_size = 0;
    return cap;
}

SourceAdapterFailure FakeSourceAdapterService::build_local_file_failure() {
    SourceAdapterFailure failure;
    failure.metadata.schema_version = "1.1";
    failure.adapter_id = "local-file-001";
    failure.failure_kind = "";
    failure.failure_reason = "";
    failure.is_recoverable = false;
    failure.retry_after_ms = 0;
    return failure;
}

// --- WebDAV Adapter ---

SourceAdapterIdentity FakeSourceAdapterService::build_webdav_identity() {
    SourceAdapterIdentity id;
    id.metadata.schema_version = "1.1";
    id.adapter_id = "webdav-001";
    id.adapter_kind = "webdav";
    id.protocol = "dav";
    id.host = "dav.example.com";
    id.requires_auth = true;
    id.credential_scope = "webdav-basic";
    return id;
}

SourceAdapterCapability FakeSourceAdapterService::build_webdav_capability() {
    SourceAdapterCapability cap;
    cap.metadata.schema_version = "1.1";
    cap.adapter_id = "webdav-001";
    cap.supports_range_read = true;
    cap.supports_seek = true;
    cap.supports_list_directory = true;
    cap.supports_byte_range_cache = true;
    cap.supports_connection_reuse = true;
    cap.max_range_chunk_size = 1048576;  // 1MB
    return cap;
}

SourceAdapterFailure FakeSourceAdapterService::build_webdav_failure() {
    SourceAdapterFailure failure;
    failure.metadata.schema_version = "1.1";
    failure.adapter_id = "webdav-001";
    failure.failure_kind = "";
    failure.failure_reason = "";
    failure.is_recoverable = false;
    failure.retry_after_ms = 0;
    return failure;
}

// --- AList Adapter ---

SourceAdapterIdentity FakeSourceAdapterService::build_alist_identity() {
    SourceAdapterIdentity id;
    id.metadata.schema_version = "1.1";
    id.adapter_id = "alist-001";
    id.adapter_kind = "alist";
    id.protocol = "https";
    id.host = "alist.example.com";
    id.requires_auth = true;
    id.credential_scope = "alist-token";
    return id;
}

SourceAdapterCapability FakeSourceAdapterService::build_alist_capability() {
    SourceAdapterCapability cap;
    cap.metadata.schema_version = "1.1";
    cap.adapter_id = "alist-001";
    cap.supports_range_read = true;
    cap.supports_seek = true;
    cap.supports_list_directory = true;
    cap.supports_byte_range_cache = true;
    cap.supports_connection_reuse = true;
    cap.max_range_chunk_size = 4194304;  // 4MB
    return cap;
}

SourceAdapterFailure FakeSourceAdapterService::build_alist_failure() {
    SourceAdapterFailure failure;
    failure.metadata.schema_version = "1.1";
    failure.adapter_id = "alist-001";
    failure.failure_kind = "";
    failure.failure_reason = "";
    failure.is_recoverable = false;
    failure.retry_after_ms = 0;
    return failure;
}

// --- Emby Adapter ---

SourceAdapterIdentity FakeSourceAdapterService::build_emby_identity() {
    SourceAdapterIdentity id;
    id.metadata.schema_version = "1.1";
    id.adapter_id = "emby-001";
    id.adapter_kind = "emby";
    id.protocol = "https";
    id.host = "emby.example.com";
    id.requires_auth = true;
    id.credential_scope = "emby-session";
    return id;
}

SourceAdapterCapability FakeSourceAdapterService::build_emby_capability() {
    SourceAdapterCapability cap;
    cap.metadata.schema_version = "1.1";
    cap.adapter_id = "emby-001";
    cap.supports_range_read = true;
    cap.supports_seek = true;
    cap.supports_list_directory = false;
    cap.supports_byte_range_cache = false;
    cap.supports_connection_reuse = true;
    cap.max_range_chunk_size = 2097152;  // 2MB
    return cap;
}

SourceAdapterFailure FakeSourceAdapterService::build_emby_failure() {
    SourceAdapterFailure failure;
    failure.metadata.schema_version = "1.1";
    failure.adapter_id = "emby-001";
    failure.failure_kind = "";
    failure.failure_reason = "";
    failure.is_recoverable = false;
    failure.retry_after_ms = 0;
    return failure;
}

// --- Failure Scenarios ---

SourceAdapterFailure FakeSourceAdapterService::build_connection_failure() {
    SourceAdapterFailure failure;
    failure.metadata.schema_version = "1.1";
    failure.adapter_id = "failed-001";
    failure.failure_kind = "connection_lost";
    failure.failure_reason = "Server unreachable after 3 retries";
    failure.is_recoverable = true;
    failure.retry_after_ms = 5000;
    return failure;
}

SourceAdapterFailure FakeSourceAdapterService::build_auth_failure() {
    SourceAdapterFailure failure;
    failure.metadata.schema_version = "1.1";
    failure.adapter_id = "failed-001";
    failure.failure_kind = "auth_failed";
    failure.failure_reason = "Invalid credentials, HTTP 401";
    failure.is_recoverable = false;
    failure.retry_after_ms = 0;
    return failure;
}

SourceAdapterFailure FakeSourceAdapterService::build_timeout_failure() {
    SourceAdapterFailure failure;
    failure.metadata.schema_version = "1.1";
    failure.adapter_id = "failed-001";
    failure.failure_kind = "timeout";
    failure.failure_reason = "Read timeout after 30000ms";
    failure.is_recoverable = true;
    failure.retry_after_ms = 10000;
    return failure;
}

}  // namespace kivo::cinema_engine
