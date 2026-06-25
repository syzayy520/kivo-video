#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Symbol management policy.
// Defines rules for symbol generation, upload, and retention.
struct SymbolManagementPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    bool symbol_server_enabled{true};  // Whether symbol server is enabled
    bool symbol_upload_required{true};  // Whether symbol upload is required
    int retention_days{365};  // Symbol retention period
    bool compression_enabled{true};  // Whether to compress symbols
    bool public_symbols_only{false};  // Whether to publish only public symbols
};

}  // namespace kivo::cinema_engine