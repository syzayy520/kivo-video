#pragma once

namespace kivo::video_plane::capability {

struct P5CapabilityRegistryContract {
    // P8 MAY read: P5CapabilitySnapshot.user_visible_status
    // P8 MAY read: P5CapabilitySnapshot.technical_detail_code
    // P8 MAY read: P5CapabilitySnapshot.capability_id
    // P8 MAY read: P5CapabilitySnapshot.domain
    //
    // P8 MUST NOT read: CapabilitySupportState internals
    // P8 MUST NOT read: CapabilityVerificationScope internals
    // P8 MUST NOT read: CapabilityPolicyState internals
    // P8 MUST NOT read: CapabilityRuntimeState internals
    // P8 MUST NOT read: sub-state-machine transitions
    //
    // All capability lookup MUST go through P5CapabilityRegistry
    // No component may self-declare capability outside registry
};

}  // namespace kivo::video_plane::capability
