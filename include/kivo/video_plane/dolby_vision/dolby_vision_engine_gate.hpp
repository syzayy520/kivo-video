// dolby_vision_engine_gate.hpp — GATE-013: P5A Dolby Vision engine gate
#pragma once
#include <string>

namespace kivo::video_plane::dolby_vision {

enum class DolbyVisionEngineAvailability {
    NotChecked,                             // engine availability not yet checked
    Available,                              // DV engine available
    NotAvailable,                           // DV engine not available
    Failed                                  // engine check failed
};

struct DolbyVisionEngineGate {
    DolbyVisionEngineAvailability availability{DolbyVisionEngineAvailability::NotChecked};
    std::string engine_version;             // detected engine version
    std::string gate_reason;                // reason for gate decision
    bool fail_closed{true};                 // fail-closed if engine not available
    bool operator==(DolbyVisionEngineGate const&) const = default;
};

}  // namespace kivo::video_plane::dolby_vision
