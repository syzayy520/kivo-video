#include "kivo/cinema_engine/security_core/fake_network_security_service.hpp"

namespace kivo::cinema_engine {

// --- Credential Policies ---

CredentialPolicy FakeNetworkSecurityService::build_strict_credential_policy() {
    CredentialPolicy policy;
    policy.metadata.schema_version = "1.1";
    policy.policy_id = "credential-strict-001";
    policy.credential_storage = "memory_only";
    policy.redact_credentials_in_logs = true;
    policy.redact_credentials_in_inspector = true;
    policy.allow_credential_injection = true;
    policy.injection_method = "header";
    policy.credential_rotation_policy = "session_bound";
    return policy;
}

CredentialPolicy FakeNetworkSecurityService::build_relaxed_credential_policy() {
    CredentialPolicy policy;
    policy.metadata.schema_version = "1.1";
    policy.policy_id = "credential-relaxed-001";
    policy.credential_storage = "encrypted_file";
    policy.redact_credentials_in_logs = true;
    policy.redact_credentials_in_inspector = false;
    policy.allow_credential_injection = true;
    policy.injection_method = "basic_auth";
    policy.credential_rotation_policy = "never";
    return policy;
}

// --- TLS Policies ---

TlsPolicy FakeNetworkSecurityService::build_strict_tls_policy() {
    TlsPolicy policy;
    policy.metadata.schema_version = "1.1";
    policy.policy_id = "tls-strict-001";
    policy.min_tls_version = "1.3";
    policy.verify_certificates = true;
    policy.allow_self_signed = false;
    policy.enable_certificate_pinning = true;
    policy.tls_fallback = "none";
    return policy;
}

TlsPolicy FakeNetworkSecurityService::build_permissive_tls_policy() {
    TlsPolicy policy;
    policy.metadata.schema_version = "1.1";
    policy.policy_id = "tls-permissive-001";
    policy.min_tls_version = "1.2";
    policy.verify_certificates = true;
    policy.allow_self_signed = true;
    policy.enable_certificate_pinning = false;
    policy.tls_fallback = "none";
    return policy;
}

// --- SSRF Policies ---

SsrfPolicy FakeNetworkSecurityService::build_strict_ssrf_policy() {
    SsrfPolicy policy;
    policy.metadata.schema_version = "1.1";
    policy.policy_id = "ssrf-strict-001";
    policy.block_private_ips = true;
    policy.block_localhost = true;
    policy.block_metadata_endpoints = true;
    policy.allowed_hosts = {"api.example.com", "cdn.example.com"};
    return policy;
}

SsrfPolicy FakeNetworkSecurityService::build_permissive_ssrf_policy() {
    SsrfPolicy policy;
    policy.metadata.schema_version = "1.1";
    policy.policy_id = "ssrf-permissive-001";
    policy.block_private_ips = false;
    policy.block_localhost = false;
    policy.block_metadata_endpoints = false;
    policy.allowed_hosts = {"*"};
    return policy;
}

}  // namespace kivo::cinema_engine
