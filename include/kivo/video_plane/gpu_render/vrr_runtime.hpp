// vrr_runtime.hpp -- P5F F7: VRR runtime (Eligible/Attempted/ActiveTelemetryConfirmed)
// ActiveTelemetryConfirmed allowed sources: trusted platform telemetry / OS API / lab measurement
// Forbidden sources: tearing flag alone / monitor marketing name alone / refresh-rate range alone
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_render {

enum class VrrRuntimeState {
    NotInitialized,
    Unsupported,
    Eligible,
    Attempted,
    ActiveTelemetryConfirmed,
    FallbackToFixedRefresh,
    DisabledByPolicy,
    Failed
};

enum class VrrTelemetrySource {
    None,
    TrustedPlatformTelemetry,
    OsApiReport,
    LabMeasurement,
    TearingFlagAlone,          // forbidden
    MonitorMarketingNameAlone, // forbidden
    RefreshRateRangeAlone      // forbidden
};

struct VrrRuntimeResult {
    VrrRuntimeState state{VrrRuntimeState::NotInitialized};
    VrrTelemetrySource telemetry_source{VrrTelemetrySource::None};
    std::string reason;
    bool is_confirmed() const {
        return state == VrrRuntimeState::ActiveTelemetryConfirmed;
    }
};

class FakeVrrRuntime {
public:
    VrrRuntimeResult check_eligibility(bool vrr_capable, uint32_t min_hz, uint32_t max_hz) {
        VrrRuntimeResult r;
        if (!vrr_capable || min_hz == 0 || max_hz == 0) {
            r.state = VrrRuntimeState::Unsupported;
            r.reason = "not_vrr_capable";
            return r;
        }
        r.state = VrrRuntimeState::Eligible;
        r.reason = "vrr_capable";
        min_hz_ = min_hz;
        max_hz_ = max_hz;
        return r;
    }

    VrrRuntimeResult attempt() {
        VrrRuntimeResult r;
        if (min_hz_ == 0) {
            r.state = VrrRuntimeState::NotInitialized;
            r.reason = "not_checked";
            return r;
        }
        r.state = VrrRuntimeState::Attempted;
        r.reason = "attempted";
        return r;
    }

    // Confirm with telemetry source -- 3 allowed, 3 forbidden
    VrrRuntimeResult confirm_active(VrrTelemetrySource source) {
        VrrRuntimeResult r;
        switch (source) {
            case VrrTelemetrySource::TrustedPlatformTelemetry:
            case VrrTelemetrySource::OsApiReport:
            case VrrTelemetrySource::LabMeasurement:
                r.state = VrrRuntimeState::ActiveTelemetryConfirmed;
                r.telemetry_source = source;
                r.reason = "confirmed_by_valid_source";
                break;
            case VrrTelemetrySource::TearingFlagAlone:
            case VrrTelemetrySource::MonitorMarketingNameAlone:
            case VrrTelemetrySource::RefreshRateRangeAlone:
                r.state = VrrRuntimeState::FallbackToFixedRefresh;
                r.telemetry_source = source;
                r.reason = "forbidden_source_cannot_confirm";
                break;
            default:
                r.state = VrrRuntimeState::Failed;
                r.reason = "invalid_source";
                break;
        }
        return r;
    }

private:
    uint32_t min_hz_{0};
    uint32_t max_hz_{0};
};

}  // namespace kivo::video_plane::gpu_render
