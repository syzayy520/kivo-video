#pragma once

#include "kivo/video_plane/sandbox/sandbox_broker_message_kind.hpp"
#include <string>

namespace kivo::video_plane::sandbox {

struct SandboxBrokerEvidence {
    std::string evidence_id;
    std::string broker_state;
    SandboxBrokerMessageKind last_message_kind{SandboxBrokerMessageKind::OpenStream};
    std::string detail_code;
    bool operator==(SandboxBrokerEvidence const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
