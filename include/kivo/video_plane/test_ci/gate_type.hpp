// gate_type.hpp — GATE-030: P5A CI gate type
#pragma once
#include <string>

namespace kivo::video_plane::test_ci {

enum class GateType {
    Contract,                               // contract compilation gate
    Unit,                                   // unit test gate
    Integration,                            // integration test gate
    Architecture,                           // architecture guard gate
    Security,                               // security scan gate
    Performance,                            // performance benchmark gate
    Fuzz                                    // fuzz test gate
};

struct GateDescriptor {
    GateType type{GateType::Contract};
    std::string gate_name;                  // human-readable gate name
    bool is_blocking{true};                 // gate failure blocks CI
    std::string command;                    // command to run gate
    bool operator==(GateDescriptor const&) const = default;
};

}  // namespace kivo::video_plane::test_ci
