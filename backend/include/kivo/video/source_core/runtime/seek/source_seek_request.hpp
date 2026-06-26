#pragma once

#include "kivo/video/source_core/runtime/session/source_session.hpp"

#include <cstdint>

namespace kivo::video::source_core {

enum class SeekOrigin : std::uint8_t {
    begin = 0,
    current = 1,
    end = 2
};

/// Seek request bound to a SourceSessionId.
struct SourceSeekRequest {
    SourceSessionId session_id;
    std::int64_t offset{0};
    SeekOrigin origin{SeekOrigin::begin};
};

}  // namespace kivo::video::source_core
