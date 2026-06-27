#pragma once

#include <string>

namespace kivo::video_plane::capability {

struct CapabilityEvidenceLink {
    std::string evidence_id;
    std::string evidence_kind;
    std::string gate_stage;
    bool operator==(CapabilityEvidenceLink const&) const = default;
};

}  // namespace kivo::video_plane::capability
