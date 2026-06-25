#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// AList server profile discovered via API probing.
struct AListServerProfile {
    ContractMetadata metadata;
    std::string profile_id;
    std::string server_version;      // e.g. "alist/3.0"
    std::string site_name;           // Configured site name
    bool enable_sign{false};         // Whether signing is enabled
    bool enable_web_dav{false};      // Whether WebDAV is enabled
    bool enable_sso{false};          // Whether SSO is enabled
    std::string auth_method;         // "token", "password", "sso"
    int64_t max_file_size{0};        // Maximum uploadable file size
    int64_t total_storage{0};        // Total available storage
};

}  // namespace kivo::cinema_engine
