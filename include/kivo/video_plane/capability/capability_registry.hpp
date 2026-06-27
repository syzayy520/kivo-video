#pragma once

#include "kivo/video_plane/capability/capability_snapshot.hpp"
#include "kivo/video_plane/capability/capability_query.hpp"
#include "kivo/video_plane/capability/capability_domain_id.hpp"
#include <vector>

namespace kivo::video_plane::capability {

struct P5CapabilityRegistry {
    // 25 capability domains, each with get/set/query
    // Registry is the SINGLE AUTHORITY for capability state
    // No component self-declares capability outside registry

    static P5CapabilitySnapshot get(P5CapabilityQuery const& query);
    static void register_capability(P5CapabilitySnapshot snapshot);
    static std::vector<P5CapabilitySnapshot> list_all();
    static std::vector<P5CapabilitySnapshot> list_by_domain(CapabilityDomainId domain);
};

}  // namespace kivo::video_plane::capability
