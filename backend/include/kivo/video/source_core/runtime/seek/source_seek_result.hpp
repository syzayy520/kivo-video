#pragma once

#include "kivo/video/source_core/contracts/error/source_error.hpp"

#include <cstdint>
#include <optional>

namespace kivo::video::source_core {

/// Seek result: new_position is undefined (nullopt) when error exists.
struct SourceSeekResult {
    std::optional<std::uint64_t> new_position;
    SourceError error;  // is_ok() when no error

    bool is_success() const { return error.is_ok(); }
};

}  // namespace kivo::video::source_core
