#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/source_core/fake_alist_profile_service.hpp"

using namespace kivo::cinema_engine;

// P2-034: AList Provider Behavior Profile tests

void test_standard_profile() {
    auto p = FakeAListProfileService::build_standard_profile();
    assert(p.metadata.schema_version == "1.1");
    assert(p.profile_id == "alist-standard-001");
    assert(p.server_version == "alist/3.0");
    assert(p.enable_web_dav == true);
    assert(p.auth_method == "token");
    assert(p.max_file_size == 10737418240);
    std::cout << "PASS: test_standard_profile" << std::endl;
}

void test_webdav_enabled_profile() {
    auto p = FakeAListProfileService::build_webdav_enabled_profile();
    assert(p.enable_sign == true);
    assert(p.enable_web_dav == true);
    assert(p.enable_sso == true);
    assert(p.auth_method == "sso");
    std::cout << "PASS: test_webdav_enabled_profile" << std::endl;
}

void test_restricted_profile() {
    auto p = FakeAListProfileService::build_restricted_profile();
    assert(p.enable_web_dav == false);
    assert(p.enable_sso == false);
    assert(p.max_file_size == 1073741824);
    std::cout << "PASS: test_restricted_profile" << std::endl;
}

void test_full_api_behavior() {
    auto b = FakeAListProfileService::build_full_api_behavior();
    assert(b.supports_list == true);
    assert(b.supports_get == true);
    assert(b.supports_search == true);
    assert(b.supports_rename == true);
    assert(b.supports_move == true);
    assert(b.supports_copy == true);
    assert(b.supports_delete == true);
    assert(b.supports_mkdir == true);
    assert(b.supports_link_generation == true);
    std::cout << "PASS: test_full_api_behavior" << std::endl;
}

void test_readonly_api_behavior() {
    auto b = FakeAListProfileService::build_readonly_api_behavior();
    assert(b.supports_list == true);
    assert(b.supports_get == true);
    assert(b.supports_search == true);
    assert(b.supports_rename == false);
    assert(b.supports_move == false);
    assert(b.supports_delete == false);
    assert(b.supports_link_generation == true);
    std::cout << "PASS: test_readonly_api_behavior" << std::endl;
}

void test_minimal_api_behavior() {
    auto b = FakeAListProfileService::build_minimal_api_behavior();
    assert(b.supports_list == true);
    assert(b.supports_get == true);
    assert(b.supports_search == false);
    assert(b.supports_link_generation == false);
    std::cout << "PASS: test_minimal_api_behavior" << std::endl;
}

void test_auth_expired_failure() {
    auto f = FakeAListProfileService::build_auth_expired_failure();
    assert(f.failure_kind == AListFailureKind::AuthExpired);
    assert(f.is_recoverable == true);
    assert(f.retry_after_ms == 0);
    std::cout << "PASS: test_auth_expired_failure" << std::endl;
}

void test_rate_limited_failure() {
    auto f = FakeAListProfileService::build_rate_limited_failure();
    assert(f.failure_kind == AListFailureKind::RateLimited);
    assert(f.retry_after_ms == 60000);
    assert(f.is_recoverable == true);
    std::cout << "PASS: test_rate_limited_failure" << std::endl;
}

void test_backend_unavailable_failure() {
    auto f = FakeAListProfileService::build_backend_unavailable_failure();
    assert(f.failure_kind == AListFailureKind::BackendUnavailable);
    assert(f.retry_after_ms == 300000);
    assert(f.is_recoverable == true);
    std::cout << "PASS: test_backend_unavailable_failure" << std::endl;
}

void test_permission_denied_failure() {
    auto f = FakeAListProfileService::build_permission_denied_failure();
    assert(f.failure_kind == AListFailureKind::PermissionDenied);
    assert(f.is_recoverable == false);
    std::cout << "PASS: test_permission_denied_failure" << std::endl;
}

void test_not_found_failure() {
    auto f = FakeAListProfileService::build_not_found_failure();
    assert(f.failure_kind == AListFailureKind::NotFound);
    assert(f.is_recoverable == false);
    std::cout << "PASS: test_not_found_failure" << std::endl;
}

int main() {
    // Server profile tests
    test_standard_profile();
    test_webdav_enabled_profile();
    test_restricted_profile();

    // API behavior tests
    test_full_api_behavior();
    test_readonly_api_behavior();
    test_minimal_api_behavior();

    // Failure tests
    test_auth_expired_failure();
    test_rate_limited_failure();
    test_backend_unavailable_failure();
    test_permission_denied_failure();
    test_not_found_failure();

    std::cout << "All P2-034 AList Provider Behavior Profile tests passed." << std::endl;
    return 0;
}
