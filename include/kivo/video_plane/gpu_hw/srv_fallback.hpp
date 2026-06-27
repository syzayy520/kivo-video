// srv_fallback.hpp -- P5G G8: SRV creation failure fallback
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_hw {

enum class SrvFallbackAction {
    None,
    RetryWithDifferentFormat,
    FallbackToCpuCopyback,
    FailToUpperLayer
};

struct SrvFallbackResult {
    SrvFallbackAction action{SrvFallbackAction::None};
    bool fallback_succeeded{false};
    std::string evidence_id;
    std::string reason;
};

class FakeSrvFallbackHandler {
public:
    SrvFallbackResult handle_failure(const std::string& failure_reason) {
        SrvFallbackResult r;
        r.reason = failure_reason;
        if (failure_reason == "format_unsupported") {
            r.action = SrvFallbackAction::RetryWithDifferentFormat;
            r.fallback_succeeded = true; // simulate success
            r.evidence_id = "srv_fallback_format_change";
        } else if (failure_reason == "device_removed") {
            r.action = SrvFallbackAction::FailToUpperLayer;
            r.fallback_succeeded = false;
            r.evidence_id = "srv_fallback_device_removed_fail";
        } else {
            r.action = SrvFallbackAction::FallbackToCpuCopyback;
            r.fallback_succeeded = true;
            r.evidence_id = "srv_fallback_cpu_copyback";
        }
        return r;
    }
};

}  // namespace kivo::video_plane::gpu_hw
