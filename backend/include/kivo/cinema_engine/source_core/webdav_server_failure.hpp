#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

enum class WebDavFailureKind {
    ConnectionRefused,
    AuthenticationRequired,
    MethodNotAllowed,
    PropfindUnsupported,
    RangeNotSupported,
    ContentLengthUnknown,
    ServerTimeout,
    SslCertificateError
};

// Failure modes specific to WebDAV probing.
struct WebDavServerFailure {
    ContractMetadata metadata;
    WebDavFailureKind failure_kind{WebDavFailureKind::ConnectionRefused};
    std::string failure_reason;
    bool is_recoverable{false};
};

}  // namespace kivo::cinema_engine
