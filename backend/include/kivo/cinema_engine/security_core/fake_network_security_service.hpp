#pragma once

#include "credential_policy.hpp"
#include "tls_policy.hpp"
#include "ssrf_policy.hpp"

namespace kivo::cinema_engine {

// Provides deterministic security policy results for testing
class FakeNetworkSecurityService {
public:
    // Credential policies
    static CredentialPolicy build_strict_credential_policy();
    static CredentialPolicy build_relaxed_credential_policy();

    // TLS policies
    static TlsPolicy build_strict_tls_policy();
    static TlsPolicy build_permissive_tls_policy();

    // SSRF policies
    static SsrfPolicy build_strict_ssrf_policy();
    static SsrfPolicy build_permissive_ssrf_policy();
};

}  // namespace kivo::cinema_engine
