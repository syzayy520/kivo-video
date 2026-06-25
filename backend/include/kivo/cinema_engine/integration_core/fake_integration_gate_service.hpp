#pragma once

#include "kivo/cinema_engine/integration_core/integration_gate_request.hpp"
#include "kivo/cinema_engine/integration_core/integration_gate_result.hpp"
#include "kivo/cinema_engine/integration_core/integration_gate_policy.hpp"
#include "kivo/cinema_engine/integration_core/subphase_status.hpp"

namespace kivo::cinema_engine {

// Fake service for integration gate testing.
struct FakeIntegrationGateService {
    // Request builders
    static IntegrationGateRequest build_passing_gate_request();
    static IntegrationGateRequest build_failing_gate_request();
    static IntegrationGateRequest build_strict_mode_request();

    // Result builders
    static IntegrationGateResult build_ready_result();
    static IntegrationGateResult build_not_ready_result();
    static IntegrationGateResult build_conditional_result();
    static IntegrationGateResult build_partial_result();

    // Subphase status builders
    static SubphaseStatus build_p2_1_status();
    static SubphaseStatus build_p2_1_5_status();
    static SubphaseStatus build_p2_2_status();
    static SubphaseStatus build_p2_3_status();

    // Policy builder
    static IntegrationGatePolicy build_default_policy();
};

}  // namespace kivo::cinema_engine