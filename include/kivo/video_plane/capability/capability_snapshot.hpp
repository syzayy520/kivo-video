#pragma once

#include "kivo/video_plane/capability/capability_domain_id.hpp"
#include "kivo/video_plane/capability/capability_support_state.hpp"
#include "kivo/video_plane/capability/capability_policy_state.hpp"
#include "kivo/video_plane/capability/capability_runtime_state.hpp"
#include "kivo/video_plane/capability/capability_verification_scope.hpp"
#include <string>

namespace kivo::video_plane::capability {

struct P5CapabilitySnapshot {
    std::string capability_id;
    CapabilityDomainId domain{CapabilityDomainId::Demux};
    CapabilitySupportState support_state{CapabilitySupportState::DeclaredContract};
    CapabilityVerificationScope verification_scope;
    CapabilityPolicyState policy_state;
    CapabilityRuntimeState runtime_state;
    std::string last_probe_result;
    std::string evidence_id;
    std::string failure_reason;
    std::string user_visible_status;     // P8-readable
    std::string technical_detail_code;   // P8-readable
    bool operator==(P5CapabilitySnapshot const&) const = default;
};

}  // namespace kivo::video_plane::capability
