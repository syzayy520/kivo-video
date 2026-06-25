#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Software Bill of Materials record.
// Captures component information, licensing, and vulnerability status.
struct SbomRecord {
    ContractMetadata metadata;
    std::string record_id;  // Unique record identifier
    std::string component_name;  // Component name
    std::string component_version;  // Component version
    std::string supplier;  // Component supplier
    std::string license;  // License identifier
    std::string hash;  // Component hash (SHA256)
    bool is_vulnerable{false};  // Whether component has known vulnerabilities
};

}  // namespace kivo::cinema_engine