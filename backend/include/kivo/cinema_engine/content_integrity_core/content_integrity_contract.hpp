#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// ContentIntegrityContract verifies that content has not been tampered with.
// Used for cache validation and source identity verification.
struct ContentIntegrityContract {
    ContractMetadata metadata;
    std::string integrity_id;
    std::string source_id;
    std::string hash_algorithm;  // "sha256", "xxhash", "none"
    std::string expected_hash;
    std::string actual_hash;
    bool integrity_verified{false};
    bool requires_verification{false};
};

}  // namespace kivo::cinema_engine
