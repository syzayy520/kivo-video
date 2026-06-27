#pragma once

namespace kivo::video_plane::capability {

enum class CapabilitySupportState {
    DeclaredContract,          // contract exists, not yet verified
    ImplementedButNotVerified, // implementation exists, not verified
    VerifiedInLab,             // verified in lab environment
    ProductionEnabled,         // enabled in production
    ExperimentalOnly,          // experimental, behind flag
    BoundaryOnly,              // boundary-only, no real backend
    DisabledByPolicy,          // disabled by policy
    Unsupported,               // unsupported on this platform
    FailClosed,                // failed, closed to prevent harm
    Degraded                   // degraded, reduced functionality
};

}  // namespace kivo::video_plane::capability
