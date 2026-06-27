// backend_risk_register.hpp — GATE-006: P5A third-party backend risk register (10 fields)
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::fuzz {

struct P5ThirdPartyBackendRiskRegister {
    std::string backend_name;               // e.g. "FFmpeg avformat", "D3D11VA"
    std::string risk_category;              // "parser", "decoder", "gpu", "ipc"
    std::string risk_description;           // human-readable risk description
    std::string mitigation_strategy;        // mitigation strategy description
    std::string fuzz_target;                // associated fuzz target name
    std::string regression_corpus;          // associated regression corpus path
    std::string expected_failure_mode;      // bounded failure mode
    std::string gate_stage;                 // which CI gate stage verifies this
    bool sandbox_required{true};            // must run inside sandbox
    bool is_active{true};                   // risk is currently active
    bool operator==(P5ThirdPartyBackendRiskRegister const&) const = default;
};

}  // namespace kivo::video_plane::fuzz
