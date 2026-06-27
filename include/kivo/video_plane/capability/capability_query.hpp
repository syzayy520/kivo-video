#pragma once

#include "kivo/video_plane/capability/capability_domain_id.hpp"
#include <string>

namespace kivo::video_plane::capability {

struct P5CapabilityQuery {
    std::string capability_id;
    CapabilityDomainId domain{CapabilityDomainId::Demux};
    bool operator==(P5CapabilityQuery const&) const = default;
};

}  // namespace kivo::video_plane::capability
