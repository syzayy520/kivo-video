#pragma once

#include "kivo/video/source_core/contracts/error/source_error.hpp"

#include <string>

namespace kivo::video::providers::local_file {

/// Validate a local file path. Returns SourceError::ok() if valid.
source_core::SourceError validate_path(const std::string& path);

/// Check if path points to a regular file. Sets out_size and out_mtime on success.
source_core::SourceError probe_file(
    const std::string& path,
    std::uint64_t& out_size,
    std::uint64_t& out_mtime);

}  // namespace kivo::video::providers::local_file
