// formal_state_violation.hpp — GATE-029: P5A formal state violation
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::state_monitor {

struct FormalStateViolation {
    std::string state_machine_name;         // which state machine violated
    std::string expected_state;             // expected state
    std::string actual_state;               // actual state
    std::string violation_description;      // description of violation
    uint64_t timestamp{0};
    bool operator==(FormalStateViolation const&) const = default;
};

}  // namespace kivo::video_plane::state_monitor
