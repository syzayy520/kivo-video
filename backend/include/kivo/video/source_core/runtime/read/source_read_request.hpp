#pragma once

#include "kivo/video/source_core/runtime/session/source_session.hpp"

#include <cstdint>

namespace kivo::video::source_core {

/// Read request bound to a SourceSessionId.
struct SourceReadRequest {
    SourceSessionId session_id;
    std::uint64_t offset{0};
    std::uint64_t length{0};
};

}  // namespace kivo::video::source_core
