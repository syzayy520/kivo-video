#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// LocalFileIdentity provides stable identity for local files.
// Uses file path + volume serial + file index for identity.
struct LocalFileIdentity {
    ContractMetadata metadata;
    std::string file_path;
    std::string volume_serial;
    uint64_t file_index{0};
    int64_t file_size{0};
    int64_t last_modified_timestamp{0};
    std::string file_hash;  // optional content hash
    bool identity_stable{false};
};

}  // namespace kivo::cinema_engine
