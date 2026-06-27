// formal_state_monitor.hpp — GATE-029: P5A formal state monitor
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::state_monitor {

struct FormalStateMonitor {
    std::string state_machine_name;         // monitored state machine name
    std::string current_state;              // current formal state
    uint64_t state_entered_timestamp{0};    // when current state was entered
    bool is_monitoring{false};              // monitoring is active
    bool operator==(FormalStateMonitor const&) const = default;
};

}  // namespace kivo::video_plane::state_monitor
