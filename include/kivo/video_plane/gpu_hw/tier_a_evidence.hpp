// tier_a_evidence.hpp -- P5G G12: Tier A real hardware evidence or safe-fail evidence
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_hw {

enum class TierAStatus {
    NotChecked,
    HardwareVerified,      // real hardware evidence
    SafeFail,              // no hardware -> safe fail with evidence
    MissingEvidence        // no evidence -> FAIL
};

struct TierAEvidence {
    TierAStatus status{TierAStatus::NotChecked};
    std::string evidence_id;
    std::string hardware_info;
    std::string safe_fail_reason;
    bool has_real_gpu{false};
};

class FakeTierAValidator {
public:
    TierAEvidence validate(bool has_real_gpu, const std::string& gpu_info) {
        TierAEvidence ev;
        ev.has_real_gpu = has_real_gpu;
        if (has_real_gpu) {
            ev.status = TierAStatus::HardwareVerified;
            ev.evidence_id = "tier_a_hardware_verified";
            ev.hardware_info = gpu_info;
        } else {
            ev.status = TierAStatus::SafeFail;
            ev.evidence_id = "tier_a_safe_fail_no_hardware";
            ev.safe_fail_reason = "no_real_gpu_available_safe_fail_with_evidence";
        }
        return ev;
    }

    // Check if evidence is acceptable (either verified or safe-fail)
    bool is_acceptable(TierAEvidence const& ev) const {
        return ev.status == TierAStatus::HardwareVerified ||
               ev.status == TierAStatus::SafeFail;
    }

    // Missing evidence = FAIL
    bool is_failure(TierAEvidence const& ev) const {
        return ev.status == TierAStatus::MissingEvidence ||
               ev.status == TierAStatus::NotChecked;
    }
};

}  // namespace kivo::video_plane::gpu_hw
