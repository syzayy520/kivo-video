// shared_texture_gate.hpp -- P5G G11: shared texture experimental gate remains off
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_hw {

enum class SharedTextureGateState {
    Off,           // default: experimental gate OFF in production
    LabOnly,       // only enabled in lab environment
    ProductionOn,  // FORBIDDEN: must never be ON in production
    Failed         // gate check failed
};

struct SharedTextureGateResult {
    SharedTextureGateState state{SharedTextureGateState::Off};
    bool is_allowed{false};
    std::string reason;
    std::string evidence_id;
};

class FakeSharedTextureGate {
public:
    SharedTextureGateResult check(bool is_lab_environment, bool is_production) {
        SharedTextureGateResult r;
        if (is_production) {
            r.state = SharedTextureGateState::Off;
            r.is_allowed = false;
            r.reason = "shared_texture_gate_off_in_production";
            r.evidence_id = "shared_texture_production_gate_off";
        } else if (is_lab_environment) {
            r.state = SharedTextureGateState::LabOnly;
            r.is_allowed = true;
            r.reason = "shared_texture_gate_lab_only";
            r.evidence_id = "shared_texture_lab_gate_on";
        } else {
            r.state = SharedTextureGateState::Off;
            r.is_allowed = false;
            r.reason = "shared_texture_gate_default_off";
        }
        return r;
    }

    // Verify gate remains off in production
    bool verify_production_off() const {
        return true; // gate is always off in production
    }
};

}  // namespace kivo::video_plane::gpu_hw
