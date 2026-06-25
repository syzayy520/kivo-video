#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/source_core/fake_alist_direct_link_service.hpp"

using namespace kivo::cinema_engine;

// P2-035: AList Direct Link Playback tests

void test_simple_request_construction() {
    auto r = FakeAListDirectLinkService::build_simple_request();
    assert(r.metadata.schema_version == "1.1");
    assert(r.request_id == "dl-req-001");
    assert(r.file_path == "/movies/test.mp4");
    assert(r.auth_token == "alist-token-abc123");
    assert(r.expires_in_seconds == 0);
    std::cout << "PASS: test_simple_request_construction" << std::endl;
}

void test_custom_ttl_request() {
    auto r = FakeAListDirectLinkService::build_custom_ttl_request();
    assert(r.request_id == "dl-req-002");
    assert(r.expires_in_seconds == 3600);
    assert(r.auth_token == "alist-token-def456");
    std::cout << "PASS: test_custom_ttl_request" << std::endl;
}

void test_successful_link_response() {
    auto r = FakeAListDirectLinkService::build_successful_link_response();
    assert(r.request_id == "dl-req-001");
    assert(!r.direct_link_url.empty());
    assert(r.ttl_seconds == 7200);
    assert(r.content_type == "video/mp4");
    assert(r.content_length == 1073741824);
    std::cout << "PASS: test_successful_link_response" << std::endl;
}

void test_short_ttl_response() {
    auto r = FakeAListDirectLinkService::build_short_ttl_response();
    assert(r.ttl_seconds == 300);  // 5 minutes
    assert(r.content_length == 524288000);
    std::cout << "PASS: test_short_ttl_response" << std::endl;
}

void test_initial_identity() {
    auto id = FakeAListDirectLinkService::build_initial_identity();
    assert(id.identity_id == "alist-identity-001");
    assert(id.file_path == "/movies/test.mp4");
    assert(id.content_hash == "sha256-abc123def456");
    assert(id.link_refresh_count == 0);
    assert(id.last_refresh_at == 0);
    std::cout << "PASS: test_initial_identity" << std::endl;
}

void test_identity_stability_across_refresh() {
    auto initial = FakeAListDirectLinkService::build_initial_identity();
    auto refreshed = FakeAListDirectLinkService::build_refreshed_identity();
    // Identity stays stable across link refreshes
    assert(initial.identity_id == refreshed.identity_id);
    assert(initial.file_path == refreshed.file_path);
    assert(initial.content_hash == refreshed.content_hash);
    assert(initial.first_link_generated_at == refreshed.first_link_generated_at);
    // Only refresh count and timestamp change
    assert(refreshed.link_refresh_count == 1);
    assert(refreshed.last_refresh_at > initial.last_refresh_at);
    std::cout << "PASS: test_identity_stability_across_refresh" << std::endl;
}

void test_identity_stability_multi_refresh() {
    auto initial = FakeAListDirectLinkService::build_initial_identity();
    auto multi = FakeAListDirectLinkService::build_multi_refresh_identity();
    // Identity remains stable after 5 refreshes
    assert(initial.identity_id == multi.identity_id);
    assert(initial.file_path == multi.file_path);
    assert(initial.content_hash == multi.content_hash);
    assert(multi.link_refresh_count == 5);
    std::cout << "PASS: test_identity_stability_multi_refresh" << std::endl;
}

void test_link_expired_failure() {
    auto f = FakeAListDirectLinkService::build_link_expired_failure();
    assert(f.failure_kind == AListDirectLinkFailureKind::LinkExpired);
    assert(f.is_recoverable == true);
    assert(f.retry_after_ms == 0);
    std::cout << "PASS: test_link_expired_failure" << std::endl;
}

void test_server_unreachable_failure() {
    auto f = FakeAListDirectLinkService::build_server_unreachable_failure();
    assert(f.failure_kind == AListDirectLinkFailureKind::ServerUnreachable);
    assert(f.retry_after_ms == 30000);
    assert(f.is_recoverable == true);
    std::cout << "PASS: test_server_unreachable_failure" << std::endl;
}

void test_auth_denied_failure() {
    auto f = FakeAListDirectLinkService::build_auth_denied_failure();
    assert(f.failure_kind == AListDirectLinkFailureKind::AuthDenied);
    assert(f.is_recoverable == false);
    std::cout << "PASS: test_auth_denied_failure" << std::endl;
}

int main() {
    // Request tests
    test_simple_request_construction();
    test_custom_ttl_request();

    // Response tests
    test_successful_link_response();
    test_short_ttl_response();

    // Identity tests
    test_initial_identity();
    test_identity_stability_across_refresh();
    test_identity_stability_multi_refresh();

    // Failure tests
    test_link_expired_failure();
    test_server_unreachable_failure();
    test_auth_denied_failure();

    std::cout << "All P2-035 AList Direct Link Playback tests passed." << std::endl;
    return 0;
}
