#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Parser security policy.
// Defines rules for safe parsing of untrusted media files.
struct ParserSecurityPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    uint64_t max_input_size_bytes{1073741824};  // Maximum allowed input size (1GB)
    int max_depth{32};  // Maximum nesting depth
    bool reject_unknown_boxes{true};  // Whether to reject unknown container boxes
    int timeout_ms{10000};  // Parser timeout in milliseconds
    bool require_header_signature{true};  // Whether to require file signature
};

}  // namespace kivo::cinema_engine