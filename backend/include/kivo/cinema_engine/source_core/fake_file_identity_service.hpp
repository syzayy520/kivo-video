#pragma once

#include "kivo/cinema_engine/source_core/local_file_identity.hpp"

namespace kivo::cinema_engine {

// Build a stable file identity for testing.
LocalFileIdentity build_stable_file_identity(std::string path);

// Build an unstable file identity (e.g., temp file).
LocalFileIdentity build_unstable_file_identity(std::string path);

// Build file identity with specific volume and index.
LocalFileIdentity build_file_identity_with_ids(
    std::string path, std::string volume_serial, uint64_t file_index);

}  // namespace kivo::cinema_engine
