// color_metadata_authority_policy.hpp — GATE-019: P5A color metadata authority policy
#pragma once
#include <string>

namespace kivo::video_plane::color {

enum class ColorMetadataAuthority {
    Container,                               // container metadata is authoritative
    Bitstream,                              // bitstream metadata is authoritative
    Display,                                // display metadata is authoritative
    Pipeline                                // pipeline-resolved metadata is authoritative
};

struct ColorMetadataAuthorityPolicy {
    ColorMetadataAuthority authority{ColorMetadataAuthority::Bitstream};
    std::string conflict_resolution;        // how conflicts are resolved
    bool prefer_bitstream_on_conflict{true};// prefer bitstream on conflict
    bool operator==(ColorMetadataAuthorityPolicy const&) const = default;
};

}  // namespace kivo::video_plane::color
