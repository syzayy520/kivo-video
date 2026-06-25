#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/source_core/fake_webdav_profile_service.hpp"

using namespace kivo::cinema_engine;

// P2-032: WebDAV Server Behavior Profile tests

void test_nginx_profile() {
    auto p = FakeWebDavProfileService::build_nginx_profile();
    assert(p.metadata.schema_version == "1.1");
    assert(p.profile_id == "webdav-nginx-001");
    assert(p.server_version == "nginx/1.24.0");
    assert(p.supports_range_read == true);
    assert(p.supports_head_probe == true);
    assert(p.supports_propfind == true);
    assert(p.supports_lock_unlock == false);
    assert(p.auth_method == "basic");
    assert(p.max_content_length == 5368709120);
    std::cout << "PASS: test_nginx_profile" << std::endl;
}

void test_apache_profile() {
    auto p = FakeWebDavProfileService::build_apache_profile();
    assert(p.dav_compliance == "2");
    assert(p.supports_lock_unlock == true);
    assert(p.auth_method == "digest");
    std::cout << "PASS: test_apache_profile" << std::endl;
}

void test_rclone_profile() {
    auto p = FakeWebDavProfileService::build_rclone_profile();
    assert(p.server_version == "rclone/v1.65.0");
    assert(p.max_content_length == 0);  // unlimited
    std::cout << "PASS: test_rclone_profile" << std::endl;
}

void test_synology_profile() {
    auto p = FakeWebDavProfileService::build_synology_profile();
    assert(p.supports_head_probe == false);  // Synology quirk
    assert(p.max_content_length == 1073741824);
    std::cout << "PASS: test_synology_profile" << std::endl;
}

void test_no_range_profile() {
    auto p = FakeWebDavProfileService::build_no_range_profile();
    assert(p.supports_range_read == false);
    assert(p.auth_method == "bearer");
    std::cout << "PASS: test_no_range_profile" << std::endl;
}

void test_nginx_range_behavior() {
    auto r = FakeWebDavProfileService::build_nginx_range_behavior();
    assert(r.metadata.schema_version == "1.1");
    assert(r.range_header_supported == true);
    assert(r.content_range_present == true);
    assert(r.single_byte_range_only == true);
    assert(r.chunk_size_limit == 10485760);
    assert(r.status_on_partial == 206);
    assert(r.status_on_not_satisfiable == 416);
    std::cout << "PASS: test_nginx_range_behavior" << std::endl;
}

void test_partial_range_behavior() {
    auto r = FakeWebDavProfileService::build_partial_range_behavior();
    assert(r.chunk_size_limit == 1048576);  // 1 MB limit
    assert(r.status_on_not_satisfiable == 200);  // non-standard
    std::cout << "PASS: test_partial_range_behavior" << std::endl;
}

void test_no_range_behavior() {
    auto r = FakeWebDavProfileService::build_no_range_behavior();
    assert(r.range_header_supported == false);
    assert(r.chunk_size_limit == 0);
    assert(r.status_on_partial == 200);  // ignores range
    std::cout << "PASS: test_no_range_behavior" << std::endl;
}

void test_connection_failure() {
    auto f = FakeWebDavProfileService::build_connection_failure();
    assert(f.failure_kind == WebDavFailureKind::ConnectionRefused);
    assert(f.is_recoverable == true);
    std::cout << "PASS: test_connection_failure" << std::endl;
}

void test_auth_failure() {
    auto f = FakeWebDavProfileService::build_auth_failure();
    assert(f.failure_kind == WebDavFailureKind::AuthenticationRequired);
    assert(f.is_recoverable == true);
    std::cout << "PASS: test_auth_failure" << std::endl;
}

void test_range_not_supported_failure() {
    auto f = FakeWebDavProfileService::build_range_not_supported_failure();
    assert(f.failure_kind == WebDavFailureKind::RangeNotSupported);
    assert(f.is_recoverable == false);
    std::cout << "PASS: test_range_not_supported_failure" << std::endl;
}

void test_successful_probe() {
    auto r = FakeWebDavProfileService::build_successful_probe();
    assert(r.metadata.schema_version == "1.1");
    assert(r.probe_success == true);
    assert(r.probe_latency_ms == 120);
    assert(r.server_profile.server_version == "nginx/1.24.0");
    assert(r.range_behavior.range_header_supported == true);
    assert(!r.failure.has_value());
    std::cout << "PASS: test_successful_probe" << std::endl;
}

void test_failed_probe() {
    auto r = FakeWebDavProfileService::build_failed_probe();
    assert(r.probe_success == false);
    assert(r.probe_latency_ms == 5000);
    assert(r.failure.has_value());
    assert(r.failure->failure_kind == WebDavFailureKind::ConnectionRefused);
    std::cout << "PASS: test_failed_probe" << std::endl;
}

void test_probe_result_binding() {
    auto r = FakeWebDavProfileService::build_successful_probe();
    assert(!r.profile_id.empty());
    assert(r.server_profile.profile_id == r.profile_id);
    std::cout << "PASS: test_probe_result_binding" << std::endl;
}

int main() {
    // Server profile tests
    test_nginx_profile();
    test_apache_profile();
    test_rclone_profile();
    test_synology_profile();
    test_no_range_profile();

    // Range behavior tests
    test_nginx_range_behavior();
    test_partial_range_behavior();
    test_no_range_behavior();

    // Failure tests
    test_connection_failure();
    test_auth_failure();
    test_range_not_supported_failure();

    // Probe result tests
    test_successful_probe();
    test_failed_probe();
    test_probe_result_binding();

    std::cout << "All P2-032 WebDAV Server Behavior Profile tests passed." << std::endl;
    return 0;
}
