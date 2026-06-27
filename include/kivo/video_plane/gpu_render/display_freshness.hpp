// display_freshness.hpp -- P5F F12: display capability freshness
// Expired snapshot must not claim HDR/VRR/output protection current
#pragma once
#include <cstdint>
#include <string>
#include "../gpu/display_capability_snapshot.hpp"

namespace kivo::video_plane::gpu_render {

using gpu::DisplayCapabilitySnapshot;

struct DisplayFreshnessResult {
    bool is_fresh{false};
    bool can_claim_hdr{false};
    bool can_claim_vrr{false};
    bool can_claim_output_protection{false};
    std::string reason;
};

class FakeDisplayFreshnessTracker {
public:
    void update_snapshot(DisplayCapabilitySnapshot const& snap) {
        snapshot_ = snap;
        last_update_ms_ = current_time_ms_;
    }

    void advance_time(int64_t ms) { current_time_ms_ += ms; }

    DisplayFreshnessResult check_freshness(int64_t max_age_ms = 5000) const {
        DisplayFreshnessResult r;
        int64_t age = current_time_ms_ - last_update_ms_;
        if (age <= max_age_ms) {
            r.is_fresh = true;
            r.can_claim_hdr = snapshot_.hdr_capable;
            r.can_claim_vrr = snapshot_.vrr_capable;
            r.can_claim_output_protection = true; // fresh = can claim
            r.reason = "fresh";
        } else {
            r.is_fresh = false;
            r.can_claim_hdr = false;
            r.can_claim_vrr = false;
            r.can_claim_output_protection = false;
            r.reason = "expired_cannot_claim_capabilities";
        }
        return r;
    }

    DisplayCapabilitySnapshot const& snapshot() const { return snapshot_; }
    int64_t last_update_ms() const { return last_update_ms_; }

private:
    DisplayCapabilitySnapshot snapshot_;
    int64_t current_time_ms_{0};
    int64_t last_update_ms_{0};
};

}  // namespace kivo::video_plane::gpu_render
