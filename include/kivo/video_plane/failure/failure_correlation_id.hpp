// failure_correlation_id.hpp — GATE-008: P5A failure correlation id
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::failure {

struct FailureCorrelationId {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(FailureCorrelationId const&) const = default;
};

}  // namespace kivo::video_plane::failure
