#include "kivo/cinema_engine/security_core/fake_network_security_service.hpp"
#include <cassert>
#include <iostream>

using namespace kivo::cinema_engine;

void test_strict_credential_policy() {
    auto p = FakeNetworkSecurityService::build_strict_credential_policy();
    assert(p.policy_id == "credential-strict-001");
    assert(p.credential_storage == "memory_only");
    assert(p.redact_credentials_in_logs);
    assert(p.redact_credentials_in_inspector);
    assert(p.allow_credential_injection);
    assert(p.injection_method == "header");
    assert(p.credential_rotation_policy == "session_bound");
    std::cout << "PASS: test_strict_credential_policy" << std::endl;
}

void test_relaxed_credential_policy() {
    auto p = FakeNetworkSecurityService::build_relaxed_credential_policy();
    assert(p.policy_id == "credential-relaxed-001");
    assert(p.credential_storage == "encrypted_file");
    assert(p.redact_credentials_in_logs);        // still redacted in logs
    assert(!p.redact_credentials_in_inspector);  // visible in inspector
    assert(p.credential_rotation_policy == "never");
    std::cout << "PASS: test_relaxed_credential_policy" << std::endl;
}

void test_strict_tls_policy() {
    auto p = FakeNetworkSecurityService::build_strict_tls_policy();
    assert(p.policy_id == "tls-strict-001");
    assert(p.min_tls_version == "1.3");
    assert(p.verify_certificates);
    assert(!p.allow_self_signed);
    assert(p.enable_certificate_pinning);
    assert(p.tls_fallback == "none");
    std::cout << "PASS: test_strict_tls_policy" << std::endl;
}

void test_permissive_tls_policy() {
    auto p = FakeNetworkSecurityService::build_permissive_tls_policy();
    assert(p.policy_id == "tls-permissive-001");
    assert(p.min_tls_version == "1.2");
    assert(p.verify_certificates);
    assert(p.allow_self_signed);
    assert(!p.enable_certificate_pinning);
    std::cout << "PASS: test_permissive_tls_policy" << std::endl;
}

void test_strict_ssrf_policy() {
    auto p = FakeNetworkSecurityService::build_strict_ssrf_policy();
    assert(p.policy_id == "ssrf-strict-001");
    assert(p.block_private_ips);
    assert(p.block_localhost);
    assert(p.block_metadata_endpoints);
    assert(p.allowed_hosts.size() == 2);
    assert(p.allowed_hosts[0] == "api.example.com");
    std::cout << "PASS: test_strict_ssrf_policy" << std::endl;
}

void test_permissive_ssrf_policy() {
    auto p = FakeNetworkSecurityService::build_permissive_ssrf_policy();
    assert(p.policy_id == "ssrf-permissive-001");
    assert(!p.block_private_ips);
    assert(!p.block_localhost);
    assert(!p.block_metadata_endpoints);
    assert(p.allowed_hosts.size() == 1);
    assert(p.allowed_hosts[0] == "*");
    std::cout << "PASS: test_permissive_ssrf_policy" << std::endl;
}

int main() {
    test_strict_credential_policy();
    test_relaxed_credential_policy();
    test_strict_tls_policy();
    test_permissive_tls_policy();
    test_strict_ssrf_policy();
    test_permissive_ssrf_policy();

    std::cout << "All network security policy tests passed!" << std::endl;
    return 0;
}
