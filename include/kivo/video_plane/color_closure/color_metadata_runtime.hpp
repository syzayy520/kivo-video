// color_metadata_runtime.hpp -- P5H H1+H2: color metadata authority + conflict resolution
#pragma once
#include <cstdint>
#include <string>
#include "../color/color_metadata_authority_policy.hpp"

namespace kivo::video_plane::color_closure {

using color::ColorMetadataAuthority;
using color::ColorMetadataAuthorityPolicy;

struct ColorMetadataConflictResult {
    bool has_conflict{false};
    ColorMetadataAuthority resolved_authority{ColorMetadataAuthority::Bitstream};
    std::string conflict_description;
    std::string evidence_id;
};

class FakeColorMetadataResolver {
public:
    ColorMetadataConflictResult resolve(
        ColorMetadataAuthority container_authority,
        ColorMetadataAuthority bitstream_authority,
        ColorMetadataAuthorityPolicy const& policy) {
        ColorMetadataConflictResult r;
        if (container_authority != bitstream_authority) {
            r.has_conflict = true;
            r.conflict_description = "container_vs_bitstream_mismatch";
            // Rule: bitstream codec-derived colorimetry takes priority over container
            if (policy.prefer_bitstream_on_conflict) {
                r.resolved_authority = ColorMetadataAuthority::Bitstream;
                r.evidence_id = "color_conflict_resolved_bitstream_priority";
            } else {
                r.resolved_authority = container_authority;
                r.evidence_id = "color_conflict_resolved_container_priority";
            }
        } else {
            r.resolved_authority = bitstream_authority;
            r.evidence_id = "color_metadata_no_conflict";
        }
        return r;
    }

    // User override is explicit, not metadata truth
    ColorMetadataConflictResult apply_user_override(
        ColorMetadataAuthority resolved,
        ColorMetadataAuthority user_override) {
        ColorMetadataConflictResult r;
        r.has_conflict = (resolved != user_override);
        r.resolved_authority = user_override; // user override wins
        r.conflict_description = "user_explicit_override";
        r.evidence_id = "color_user_override_applied";
        return r;
    }
};

}  // namespace kivo::video_plane::color_closure
