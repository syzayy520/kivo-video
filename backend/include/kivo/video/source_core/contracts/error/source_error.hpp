#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::source_core {

enum class SourceErrorCode : std::uint8_t {
    none = 0,
    not_found = 1,
    permission_denied = 2,
    auth_failed = 3,
    network_unreachable = 4,
    timeout = 5,
    connection_refused = 6,
    tls_error = 7,
    redirect_loop = 8,
    range_not_supported = 9,
    protocol_error = 10,
    cancelled = 11,
    invalid_request = 12,
    internal_error = 13,
    unknown = 14
};

struct SourceError {
    SourceErrorCode code{SourceErrorCode::none};
    std::string message;  // must be redacted for evidence

    bool is_ok() const { return code == SourceErrorCode::none; }
    static SourceError ok() { return SourceError{}; }
};

}  // namespace kivo::video::source_core
