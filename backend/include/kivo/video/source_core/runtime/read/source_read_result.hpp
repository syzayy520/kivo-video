#pragma once

#include "kivo/video/source_core/contracts/error/source_error.hpp"
#include <cstdint>
#include <vector>

namespace kivo::video::source_core {

struct SourceReadResult {
    std::uint64_t bytes_read{0};
    bool eof{false};
    bool would_block{false};
    SourceError error;
    std::vector<std::uint8_t> bytes;

    bool is_success() const { return error.is_ok() && !would_block; }
};

}  // namespace kivo::video::source_core
