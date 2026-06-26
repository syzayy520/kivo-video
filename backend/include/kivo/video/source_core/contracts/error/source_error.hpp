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
    session_not_found = 14,
    session_closed = 15,
    session_in_error = 16,
    stale_reference = 17,
    read_size_exceeded = 18,
    unsupported_provider = 19,
    source_changed = 20,
    seek_out_of_range = 21,
    seek_unsupported = 22,
    unknown = 23
};

struct SourceError {
    SourceErrorCode code{SourceErrorCode::none};
    std::string message;

    bool is_ok() const { return code == SourceErrorCode::none; }
    static SourceError ok() { return SourceError{}; }
};

}  // namespace kivo::video::source_core
