#pragma once

#include "kivo/video_plane/capability/capability_snapshot.hpp"
#include "kivo/video_plane/capability/capability_query.hpp"
#include <vector>

namespace kivo::video_plane::capability {

struct P5CapabilityStateResolver {
    // Resolves the canonical capability state for a query
    static P5CapabilitySnapshot resolve(
        P5CapabilityQuery const& query,
        std::vector<P5CapabilitySnapshot> const& registered);
};

}  // namespace kivo::video_plane::capability
