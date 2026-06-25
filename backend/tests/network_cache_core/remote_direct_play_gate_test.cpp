#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/network_cache_core/fake_remote_direct_play_gate_service.hpp"

using namespace kivo::cinema_engine;

// P2-038: Remote Direct Play Gate tests

void test_direct_play_request() {
    auto r = FakeRemoteDirectPlayGateService::build_direct_play_request();
    assert(r.metadata.schema_version == "1.1");
    assert(r.request_id == "gate-req-001");
    assert(r.is_direct_play == true);
    assert(r.has_credential == false);
    assert(r.cache_identity_id == "cache-identity-001");
    std::cout << "PASS: test_direct_play_request" << std::endl;
}

void test_transcode_request() {
    auto r = FakeRemoteDirectPlayGateService::build_transcode_request();
    assert(r.is_direct_play == false);
    assert(r.request_id == "gate-req-002");
    std::cout << "PASS: test_transcode_request" << std::endl;
}

void test_credential_request() {
    auto r = FakeRemoteDirectPlayGateService::build_credential_request();
    assert(r.has_credential == true);
    assert(!r.source_url.empty());
    std::cout << "PASS: test_credential_request" << std::endl;
}

void test_redirect_leak_request() {
    auto r = FakeRemoteDirectPlayGateService::build_redirect_leak_request();
    assert(r.redirect_chain.size() == 1);
    assert(r.redirect_chain[0].find("auth=") != std::string::npos);
    std::cout << "PASS: test_redirect_leak_request" << std::endl;
}

void test_passed_result() {
    auto r = FakeRemoteDirectPlayGateService::build_passed_result();
    assert(r.gate_passed == true);
    assert(r.transcode_requires_approval == false);
    assert(r.redirect_leak_detected == false);
    assert(r.cache_binding_valid == true);
    std::cout << "PASS: test_passed_result" << std::endl;
}

void test_transcode_blocked_result() {
    auto r = FakeRemoteDirectPlayGateService::build_transcode_blocked_result();
    assert(r.gate_passed == false);
    assert(r.transcode_requires_approval == true);
    assert(!r.failure_reason.empty());
    std::cout << "PASS: test_transcode_blocked_result" << std::endl;
}

void test_credential_leak_result() {
    auto r = FakeRemoteDirectPlayGateService::build_credential_leak_result();
    assert(r.gate_passed == false);
    assert(r.redirect_leak_detected == true);
    assert(!r.failure_reason.empty());
    std::cout << "PASS: test_credential_leak_result" << std::endl;
}

void test_strict_policy() {
    auto p = FakeRemoteDirectPlayGateService::build_strict_policy();
    assert(p.block_silent_transcode == true);
    assert(p.redact_credentials_in_logs == true);
    assert(p.block_credential_in_redirect == true);
    assert(p.require_cache_identity_binding == true);
    std::cout << "PASS: test_strict_policy" << std::endl;
}

void test_permissive_policy() {
    auto p = FakeRemoteDirectPlayGateService::build_permissive_policy();
    assert(p.block_silent_transcode == false);
    assert(p.redact_credentials_in_logs == false);
    assert(p.block_credential_in_redirect == false);
    assert(p.require_cache_identity_binding == false);
    std::cout << "PASS: test_permissive_policy" << std::endl;
}

void test_gate_failure_reason() {
    auto r = FakeRemoteDirectPlayGateService::build_transcode_blocked_result();
    assert(r.gate_passed == false);
    assert(r.failure_reason.find("transcode") != std::string::npos);
    std::cout << "PASS: test_gate_failure_reason" << std::endl;
}

int main() {
    // Request tests
    test_direct_play_request();
    test_transcode_request();
    test_credential_request();
    test_redirect_leak_request();

    // Result tests
    test_passed_result();
    test_transcode_blocked_result();
    test_credential_leak_result();

    // Policy tests
    test_strict_policy();
    test_permissive_policy();

    // Failure reason test
    test_gate_failure_reason();

    std::cout << "All P2-038 Remote Direct Play Gate tests passed." << std::endl;
    return 0;
}
