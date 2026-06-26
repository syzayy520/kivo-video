#pragma once

#include "kivo/video/source_core/runtime/session/source_session.hpp"

#include <cstdint>
#include <optional>

namespace kivo::video::source_core {

/// offset == nullopt: sequential read from current_offset; successful read updates current_offset.
/// offset has value: positioned read from that offset; successful read does NOT update current_offset.
struct SourceReadRequest {
    SourceSessionId session_id;
    std::optional<std::uint64_t> offset;
    std::uint64_t length{0};
};

}  // namespace kivo::video::source_core
