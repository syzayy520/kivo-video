// third_party_notice_bundle.hpp — GATE-027: P5A third party notice bundle
#pragma once
#include <string>
#include <vector>

namespace kivo::video_plane::license {

struct ThirdPartyNoticeEntry {
    std::string component_name;             // third-party component name
    std::string version;                    // component version
    std::string license_text;               // license text
    std::string source_url;                 // source URL
};

struct ThirdPartyNoticeBundle {
    std::vector<ThirdPartyNoticeEntry> entries;
    std::string bundle_version;
    bool operator==(ThirdPartyNoticeBundle const&) const = default;
};

}  // namespace kivo::video_plane::license
