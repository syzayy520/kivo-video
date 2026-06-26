#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::video::source_core {

enum class CapabilityState : std::uint8_t {
    unknown = 0,
    supported = 1,
    unsupported = 2
};

struct SourceCapabilitySnapshot {
    // Use typed state, not bare booleans.
    CapabilityState seek{CapabilityState::unknown};
    CapabilityState range_read{CapabilityState::unknown};
    CapabilityState direct_play_candidate{CapabilityState::unknown};
    CapabilityState direct_stream_candidate{CapabilityState::unknown};

    // Known length, MIME, container -- optionals (not empty strings).
    std::optional<std::uint64_t> content_length_bytes;
    std::optional<std::string> mime_type;
    std::optional<std::string> container_hint;

    // Unknown is NOT false and NOT empty string.
};

}  // namespace kivo::video::source_core
