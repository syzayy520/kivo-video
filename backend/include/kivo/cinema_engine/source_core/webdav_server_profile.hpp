#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// WebDAV server profile discovered via PROPFIND/HEAD/OPTIONS probing.
struct WebDavServerProfile {
    ContractMetadata metadata;
    std::string profile_id;
    std::string server_version;       // e.g. "nginx/1.24", "Apache/2.4.58"
    std::string dav_compliance;       // DAV compliance class ("1", "2")
    bool supports_range_read{false};
    bool supports_head_probe{false};
    bool supports_propfind{false};
    bool supports_lock_unlock{false};
    std::string auth_method;          // "none", "basic", "digest", "bearer"
    int64_t max_content_length{0};
};

}  // namespace kivo::cinema_engine
