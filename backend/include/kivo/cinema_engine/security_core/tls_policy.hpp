#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Controls TLS connection requirements for source adapters.
// Fail-closed: when verify fails, connection is refused.
struct TlsPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    std::string min_tls_version;  // "1.2", "1.3"
    bool verify_certificates{true};
    bool allow_self_signed{false};
    bool enable_certificate_pinning{false};
    std::string tls_fallback;  // "none", "downgrade_to_http"
};

}  // namespace kivo::cinema_engine
