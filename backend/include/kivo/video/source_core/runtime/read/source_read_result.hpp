#pragma once

#include "kivo/video/source_core/contracts/error/source_error.hpp"

#include <cstdint>

namespace kivo::video::source_core {

/// Read result documents would-block semantics.
struct SourceReadResult {
    std::uint64_t bytes_read{0};
    bool eof{false};
    bool would_block{false};
    SourceError error;  // is_ok() when no error

    bool is_success() const { return error.is_ok() && !would_block; }
};

}  // namespace kivo::video::source_core
