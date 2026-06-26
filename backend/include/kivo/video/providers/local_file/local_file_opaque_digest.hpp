#pragma once

#include <string>
#include <cstdint>

namespace kivo::video::providers::local_file {

/// Compute opaque digest from internal path, file size, and mtime snapshot.
/// Returns a hex digest string. Never exposes raw path outside the provider.
std::string compute_opaque_digest(
    const std::string& internal_path,
    std::uint64_t file_size,
    std::uint64_t last_write_time);

/// Extract a short prefix for display/redacted URI.
std::string digest_prefix(const std::string& digest, std::size_t len = 12);

/// Extract file extension safely.
std::string safe_extension(const std::string& path);

}  // namespace kivo::video::providers::local_file
