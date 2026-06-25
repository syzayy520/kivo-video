#include "kivo/cinema_engine/source_core/fake_source_adapter_service.hpp"
#include <cassert>
#include <iostream>

using namespace kivo::cinema_engine;

void test_local_file_identity() {
    auto id = FakeSourceAdapterService::build_local_file_identity();
    assert(id.adapter_id == "local-file-001");
    assert(id.adapter_kind == "local");
    assert(id.protocol == "file");
    assert(!id.requires_auth);
    assert(id.credential_scope.empty());
    std::cout << "PASS: test_local_file_identity" << std::endl;
}

void test_local_file_capability() {
    auto cap = FakeSourceAdapterService::build_local_file_capability();
    assert(cap.adapter_id == "local-file-001");
    assert(cap.supports_range_read);
    assert(cap.supports_seek);
    assert(!cap.supports_list_directory);
    assert(!cap.supports_byte_range_cache);
    std::cout << "PASS: test_local_file_capability" << std::endl;
}

void test_webdav_identity() {
    auto id = FakeSourceAdapterService::build_webdav_identity();
    assert(id.adapter_id == "webdav-001");
    assert(id.adapter_kind == "webdav");
    assert(id.protocol == "dav");
    assert(id.host == "dav.example.com");
    assert(id.requires_auth);
    assert(id.credential_scope == "webdav-basic");
    std::cout << "PASS: test_webdav_identity" << std::endl;
}

void test_webdav_capability() {
    auto cap = FakeSourceAdapterService::build_webdav_capability();
    assert(cap.adapter_id == "webdav-001");
    assert(cap.supports_range_read);
    assert(cap.supports_seek);
    assert(cap.supports_list_directory);
    assert(cap.supports_byte_range_cache);
    assert(cap.supports_connection_reuse);
    assert(cap.max_range_chunk_size == 1048576);
    std::cout << "PASS: test_webdav_capability" << std::endl;
}

void test_alist_identity_and_capability() {
    auto id = FakeSourceAdapterService::build_alist_identity();
    assert(id.adapter_kind == "alist");
    assert(id.protocol == "https");
    assert(id.requires_auth);

    auto cap = FakeSourceAdapterService::build_alist_capability();
    assert(cap.supports_range_read);
    assert(cap.supports_list_directory);
    assert(cap.max_range_chunk_size == 4194304);
    std::cout << "PASS: test_alist_identity_and_capability" << std::endl;
}

void test_emby_identity_and_capability() {
    auto id = FakeSourceAdapterService::build_emby_identity();
    assert(id.adapter_kind == "emby");
    assert(id.credential_scope == "emby-session");
    assert(id.requires_auth);

    auto cap = FakeSourceAdapterService::build_emby_capability();
    assert(cap.supports_range_read);
    assert(!cap.supports_list_directory);
    assert(cap.max_range_chunk_size == 2097152);
    std::cout << "PASS: test_emby_identity_and_capability" << std::endl;
}

void test_connection_failure() {
    auto f = FakeSourceAdapterService::build_connection_failure();
    assert(f.failure_kind == "connection_lost");
    assert(f.is_recoverable);
    assert(f.retry_after_ms == 5000);
    std::cout << "PASS: test_connection_failure" << std::endl;
}

void test_auth_failure() {
    auto f = FakeSourceAdapterService::build_auth_failure();
    assert(f.failure_kind == "auth_failed");
    assert(!f.is_recoverable);
    assert(f.retry_after_ms == 0);
    std::cout << "PASS: test_auth_failure" << std::endl;
}

void test_timeout_failure() {
    auto f = FakeSourceAdapterService::build_timeout_failure();
    assert(f.failure_kind == "timeout");
    assert(f.is_recoverable);
    assert(f.retry_after_ms == 10000);
    std::cout << "PASS: test_timeout_failure" << std::endl;
}

int main() {
    test_local_file_identity();
    test_local_file_capability();
    test_webdav_identity();
    test_webdav_capability();
    test_alist_identity_and_capability();
    test_emby_identity_and_capability();
    test_connection_failure();
    test_auth_failure();
    test_timeout_failure();

    std::cout << "All source adapter contract tests passed!" << std::endl;
    return 0;
}
