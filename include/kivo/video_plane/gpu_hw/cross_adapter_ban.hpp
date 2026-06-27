// cross_adapter_ban.hpp -- P5G G10: cross-adapter production ban
// Cross-adapter texture sharing is BANNED in production
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_hw {

struct CrossAdapterCheckResult {
    bool is_banned{false};
    std::string reason;
    std::string evidence_id;
};

class FakeCrossAdapterBan {
public:
    CrossAdapterCheckResult check(uint64_t decode_adapter_luid, uint64_t render_adapter_luid) {
        CrossAdapterCheckResult r;
        if (decode_adapter_luid != render_adapter_luid) {
            r.is_banned = true;
            r.reason = "cross_adapter_texture_sharing_banned_in_production";
            r.evidence_id = "cross_adapter_ban_violation";
        } else {
            r.is_banned = false;
            r.reason = "same_adapter_ok";
        }
        return r;
    }

    // Check if experimental shared texture key is allowed (only in lab, never production)
    CrossAdapterCheckResult check_experimental(bool is_production) {
        CrossAdapterCheckResult r;
        if (is_production) {
            r.is_banned = true;
            r.reason = "experimental_shared_texture_not_allowed_in_production";
            r.evidence_id = "shared_texture_experimental_production_ban";
        } else {
            r.is_banned = false;
            r.reason = "experimental_allowed_in_lab_only";
        }
        return r;
    }
};

}  // namespace kivo::video_plane::gpu_hw
